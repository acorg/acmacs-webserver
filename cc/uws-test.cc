#include <string>
#include <iostream>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <thread>
#include <queue>
#include <chrono>
#include <memory>
#include <vector>
#include <condition_variable>

#include "uws.hh"

extern template struct uWS::Group<uWS::SERVER>; // to silence clang
extern template struct uWS::WebSocket<uWS::SERVER>; // to silence clang

void report_request(uWS::HttpRequest& request);

class HttpRequestDispatcherBase
{
 public:
    inline HttpRequestDispatcherBase(uWS::Group<uWS::SERVER>* aGroup) //: mGroup(aGroup)
        {
            using namespace std::placeholders;
            aGroup->onHttpRequest(std::bind(&HttpRequestDispatcherBase::dispatcher, this, _1, _2, _3, _4, _5));
            // void onHttpConnection(std::function<void(HttpSocket<isServer> *)> handler);
            // void onHttpData(std::function<void(HttpResponse *, char *data, size_t length, size_t remainingBytes)> handler);
            // void onHttpDisconnection(std::function<void(HttpSocket<isServer> *)> handler);
            // void onCancelledHttpRequest(std::function<void(HttpResponse *)> handler);
            // void onHttpUpgrade(std::function<void(HttpSocket<isServer> *, HttpRequest)> handler);
        }
    virtual inline ~HttpRequestDispatcherBase() {}

 protected:
    virtual void dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* post_data, size_t post_data_length, size_t post_remaining_bytes) = 0;

private:
      // uWS::Group<uWS::SERVER>* mGroup;
};

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

void report_request(uWS::HttpRequest& request)
{
    std::time_t now = std::time(nullptr);
    std::cout << "Request " << std::asctime(std::localtime(&now)) << std::endl;
    // std::cout << "URL: " << request.getUrl().toString() << " HOST:" << request.getHeader("host").toString() << std::endl;
    if (request.headers) {
        for (auto* h = request.headers; *h; ++h) {
            std::cout << "  " << std::string(h->key, h->keyLength) << ": " << std::string(h->value, h->valueLength) << std::endl;
        }
    }
    std::cout << std::endl;

} // report_headers

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

class HttpRequestRedirectToHttps : public HttpRequestDispatcherBase
{
 public:
      // target port 0 means do not specify port in the new address
    inline HttpRequestRedirectToHttps(uWS::Group<uWS::SERVER>* aGroup, int aTargetPort = 0) : HttpRequestDispatcherBase{aGroup}, mTargetPort{aTargetPort} {}

 protected:
    virtual void dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* post_data, size_t post_data_length, size_t post_remaining_bytes);

 private:
    int mTargetPort;
};

// ----------------------------------------------------------------------

void HttpRequestRedirectToHttps::dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* /*post_data*/, size_t /*post_data_length*/, size_t /*post_remaining_bytes*/)
{
    // report_request(request);
    std::string host = request.getHeader("host").toString();
    host.erase(host.find(':')); // remove original port
    if (mTargetPort)
        host += ":" + std::to_string(mTargetPort);
    const std::string header = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://" + host + request.getUrl().toString() + "\r\n\r\n";
    response->write(header.c_str(), header.size());
    response->end();

} // HttpRequestRedirectToHttps::dispatcher

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

class HttpRequestDispatcher : public HttpRequestDispatcherBase
{
 public:
    using HttpRequestDispatcherBase::HttpRequestDispatcherBase;

 protected:
    virtual void dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* post_data, size_t post_data_length, size_t post_remaining_bytes);

 private:
    void get_f(uWS::HttpResponse* response, uWS::HttpRequest request, std::string path);
    std::string content_type(std::string path) const;
};

// ----------------------------------------------------------------------

void HttpRequestDispatcher::dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* /*post_data*/, size_t /*post_data_length*/, size_t /*post_remaining_bytes*/)
{
      // report_request(request);
    const std::string path = request.getUrl().toString();
    if (path.substr(0, 3) == "/f/") {
        get_f(response, request, path.substr(1));
    }
    else if (path == "/favicon.ico") {
        get_f(response, request, path.substr(1));
    }
    else {
        std::time_t now = std::time(nullptr);
        std::string reply = std::string{"<html><head><script src=\"/f/myscript.js\"></script></head><body><h1>UWS-TEST</h1><p>"} + std::asctime(std::localtime(&now)) + "</p></body></html>";
        response->end(reply.c_str(), reply.size());
    }

} // HttpRequestDispatcher::dispatcher

// ----------------------------------------------------------------------

void HttpRequestDispatcher::get_f(uWS::HttpResponse* response, uWS::HttpRequest /*request*/, std::string path)
{
    std::string reply;
    try {
        struct stat st;
        std::string filename = path;
        if (stat(filename.c_str(), &st)) {
            filename += ".gz";
            if (stat(filename.c_str(), &st))
                throw std::runtime_error(std::strerror(errno));
        }
        int fd = open(filename.c_str(), O_RDONLY);
        if (fd < 0)
            throw std::runtime_error(std::strerror(errno));
        reply.resize(static_cast<std::string::size_type>(st.st_size));
        if (read(fd, const_cast<char*>(reply.data()), reply.size()) != static_cast<ssize_t>(reply.size()))
            throw std::runtime_error(std::strerror(errno));
        std::string header;
        if (reply.size() > 2 && reply[0] == '\x1F' && reply[1] == '\x8B') { // gzip http://www.zlib.org/rfc-gzip.html
            header = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(reply.size()) + "\r\nContent-Encoding: gzip\r\nContent-Type: " + content_type(path) + "\r\n\r\n";
        }
        else {
            header = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(reply.size()) + "\r\nContent-Type: " + content_type(path) + "\r\n\r\n";
        }
        response->write(header.c_str(), header.size());
    }
    catch (std::exception& err) {
        reply = "console.error('ERROR: server cannot read " + path + ": " + err.what() + "');";
    }
    response->end(reply.c_str(), reply.size());
}

// ----------------------------------------------------------------------

std::string HttpRequestDispatcher::content_type(std::string path) const
{
    if (path.substr(path.size() - 3) == ".js")
        return "text/javascript";
    else if (path.substr(path.size() - 4) == ".ico")
        return "image/x-icon";
    else
        return "text/html";

} // HttpRequestDispatcher::content_type

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

template <typename Data> class WebSocketQueue : public std::queue<Data>
{
 public:
    using BaseQueue = std::queue<Data>;
    inline WebSocketQueue(uWS::Group<uWS::SERVER>* aGroup) : BaseQueue{}, mGroup{aGroup} {}

    inline auto& group() { return *mGroup; }

    inline void push(uWS::WebSocket<uWS::SERVER>* aWs, typename Data::HandlerPtr aHandlerPtr, std::string aMessage = std::string{})
        {
            BaseQueue::emplace(aWs, aHandlerPtr, aMessage);
            std::cout << std::this_thread::get_id() << " Q.push " << BaseQueue::size() << " " << aWs << std::endl;
            data_available();
        }

    inline Data pop()
        {
            wait_for_data();
            auto result = BaseQueue::front();
            BaseQueue::pop();
            std::cout << std::this_thread::get_id() << " Q.pop " << BaseQueue::size() << std::endl;
            return result;
        }

 private:
    uWS::Group<uWS::SERVER>* mGroup;
    std::condition_variable mNotifier;
    std::mutex mMutexForNotifier;

    void data_available()
        {
            std::unique_lock<std::mutex> lock{mMutexForNotifier};
            mNotifier.notify_one();
        }

    void wait_for_data()
        {
            while (BaseQueue::empty()) {
                std::unique_lock<std::mutex> lock{mMutexForNotifier};
                mNotifier.wait(lock, [this]() -> bool { return !this->empty(); });
            }
        }

}; // WebSocketQueue<Data>

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

template <typename Data> class WebSocketHandlers;

// runs in a thread
template <typename Data> class WebSocketHandler
{
 private:
    class DataAssigner
    {
     public:
        inline DataAssigner(Data&& aData, Data& aHandlerData) : mHandlerData(aHandlerData) { mHandlerData = std::move(aData); }
        inline ~DataAssigner() { mHandlerData.reset(); }
     private:
        Data& mHandlerData;
    };

 public:
    inline WebSocketHandler(WebSocketHandlers<Data>& aHandlers) : mHandlers{aHandlers}, mThread{new std::thread(std::bind(&WebSocketHandler::run, this))} {}
    inline ~WebSocketHandler() { std::cerr << std::this_thread::get_id() << " WebSocketHandler destruct!" << std::endl; /* kill thread? */ }

    inline void connection(Data&& aData)
        {
            DataAssigner assigner{std::move(aData), mData};
            std::cout << std::this_thread::get_id() << " " << mData.websocket() << " connection" << std::endl;
            send("hello");
        }

    inline void disconnection(Data&& aData)
        {
            std::cout << std::this_thread::get_id() << " " << mData.websocket() << " disconnection" << std::endl;
            aData.invalidate_websocket(); // aData.ws is invalid pointer! ws is already closed and most probably destroyed
            DataAssigner assigner{std::move(aData), mData};
        }

    inline void message(Data&& aData)
        {
            DataAssigner assigner{std::move(aData), mData};
            std::cout << std::this_thread::get_id() << " " << mData.websocket() << " Message \"" << mData.message << "\"" << std::endl;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(5s);
            send(mData.message); // echo
        }

    inline void websocket_disconnected(uWS::WebSocket<uWS::SERVER> *ws) { mData.websocket_disconnected(ws); }

 private:
    WebSocketHandlers<Data>& mHandlers;
    std::thread* mThread;
    Data mData;

    [[noreturn]] inline void run()
        {
              // std::cout << std::this_thread::get_id() << " run" << std::endl;
            while (true) {
                auto data = mHandlers.pop();
                (this->*data.handler)(std::move(data));
            }
        }

    inline void move_data(Data&& aData)
        {
            std::unique_lock<std::mutex> lock{mData.mMutex};
            mData = std::move(aData);
        }

    inline void send(std::string aMessage, bool aBinary = false) { mData.send(aMessage, aBinary); }
    inline void send(const char* aMessage) { mData.send(aMessage); }

}; // WebSocketHandler

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

template <typename Data> class WebSocketHandlers;

// Receives requests from web socket and adds correposning task to the queuq
template <typename Data> class WebSocketDispatcher
{
 public:
    inline WebSocketDispatcher(WebSocketQueue<Data>& aQueue, WebSocketHandlers<Data>& aHandlers) : mQueue{aQueue}, mHandlers{aHandlers}
        {
            using namespace std::placeholders;
            auto& group = aQueue.group();
            group.onConnection(std::bind(&WebSocketDispatcher::connection, this, _1, _2));
            group.onError(std::bind(&WebSocketDispatcher::error, this, _1));
            group.onDisconnection(std::bind(&WebSocketDispatcher::disconnection, this, _1, _2, _3, _4));
            group.onMessage(std::bind(&WebSocketDispatcher::message, this, _1, _2, _3, _4));
              // void onTransfer(std::function<void(WebSocket<isServer> *)> handler);
              // void onPing(std::function<void(WebSocket<isServer> *, char *, size_t)> handler);
              // void onPong(std::function<void(WebSocket<isServer> *, char *, size_t)> handler);
        }

 protected:
    inline void connection(uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest /*request*/)
        {
            std::cout << std::this_thread::get_id() << " WS " << ws << " dispatcher connection, pre queue: " << mQueue.size() << std::endl;
            mQueue.push(ws, &WebSocketHandler<Data>::connection);
            std::cout << std::this_thread::get_id() << " WS " << ws << " dispatcher connection, queue: " << mQueue.size() << std::endl;
        }

    inline void error(std::conditional<true, int, void *>::type /*user*/)
        {
            std::cerr << std::this_thread::get_id() << " WS FAILURE: Connection failed! Timeout?" << std::endl;
        }

    inline void disconnection(uWS::WebSocket<uWS::SERVER> *ws, int /*code*/, char *message, size_t length)
        {
              // immeditely tell handlers about disconnection, ws will be destroyed upon returning from this function!
            mHandlers.disconnection(ws);
            std::cout << std::this_thread::get_id() << " WS " << ws << " dispatcher disconnection, pre queue: " << mQueue.size() << std::endl;
            mQueue.push(ws, &WebSocketHandler<Data>::disconnection, {message, length});
        }

    inline void message(uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode /*opCode*/)
        {
            mQueue.push(ws, &WebSocketHandler<Data>::message, {message, length});
        }

 private:
    WebSocketQueue<Data>& mQueue;
    WebSocketHandlers<Data>& mHandlers;
};

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

// Singletone, runs in the main thread
template <typename Data> class WebSocketHandlers
{
 public:
    inline WebSocketHandlers(uWS::Group<uWS::SERVER>* aGroup, size_t aNumberOfThreads = std::thread::hardware_concurrency()) // hardware_concurrency may return 0)
        : mQueue{aGroup}, mDispatcher{mQueue, *this}, mHandlers{aNumberOfThreads > 0 ? aNumberOfThreads : 4}
        {
            std::transform(mHandlers.begin(), mHandlers.end(), mHandlers.begin(), [this](const auto&) { return std::make_shared<WebSocketHandler<Data>>(*this); });
        }

    inline Data pop() { return mQueue.pop(); }

    inline void disconnection(uWS::WebSocket<uWS::SERVER> *ws)
        {
            std::for_each(mHandlers.begin(), mHandlers.end(), [&ws](auto& handler) { handler->websocket_disconnected(ws); });
        }

 private:
    WebSocketQueue<Data> mQueue;
    WebSocketDispatcher<Data> mDispatcher;
    std::vector<std::shared_ptr<WebSocketHandler<Data>>> mHandlers;

}; // class WebSocketHandlers

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

class WebSocketData
{
 public:
    using HandlerPtr = void (WebSocketHandler<WebSocketData>::*)(WebSocketData&& aData);

    inline WebSocketData() : handler{nullptr}, ws(nullptr) {}
    inline WebSocketData(uWS::WebSocket<uWS::SERVER>* aWs, HandlerPtr aHandler, std::string aMessage) : handler{aHandler}, message{aMessage}, ws(aWs) {}
    inline WebSocketData(const WebSocketData& wsd) : handler{wsd.handler}, message{wsd.message}, mMutex{/* cannot copy */}, ws{wsd.ws} {}
    inline virtual ~WebSocketData() {}
    inline void operator=(WebSocketData&& aData) { std::unique_lock<std::mutex> lock{mMutex}; handler = aData.handler; message = aData.message; ws = aData.ws; }
    inline void reset() { std::unique_lock<std::mutex> lock{mMutex}; handler = nullptr; message.clear(); ws = nullptr; }

    HandlerPtr handler;
    std::string message;

    inline virtual void send(std::string aMessage, bool aBinary = false)
        {
            std::unique_lock<std::mutex> lock{mMutex};
            if (ws)             // can be set to nullptr to mark it disconnected
                ws->send(aMessage.c_str(), aMessage.size(), aBinary ? uWS::BINARY : uWS::TEXT);
        }

    inline virtual void send(const char* aMessage)
        {
            std::unique_lock<std::mutex> lock{mMutex};
            if (ws)
                ws->send(aMessage, strlen(aMessage), uWS::TEXT);
        }

    inline bool match(const WebSocketData& aData) const { return ws == aData.ws; }

    inline void invalidate_websocket() { ws = nullptr; }

    inline void websocket_disconnected(uWS::WebSocket<uWS::SERVER> *aWs)
        {
            std::unique_lock<std::mutex> lock{mMutex};
            if (ws == aWs) {
                std::cerr << std::this_thread::get_id() << " WS " << ws << " disconnected, nullify" << std::endl;
                invalidate_websocket();
            }
        }

    inline auto websocket() { return ws; }

 private:
    std::mutex mMutex;
    uWS::WebSocket<uWS::SERVER>* ws;

}; // class WebSocketData

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

// // void WebSocketDispatcher::disconnection_client(uWS::WebSocket<uWS::CLIENT>* ws, int code, char* message, size_t length)
// // {
// //     std::cout << "WS client disconnection: " << code << " [" << std::string(message, length) << "]" << std::endl;

// // } // WebSocketDispatcher::disconnection_client

// // ----------------------------------------------------------------------

// void WebSocketDispatcher::disconnection(uWS::WebSocket<uWS::SERVER>* ws, int code, char* message, size_t length)
// {
//     std::cout << "WS server disconnection: " << code << " [" << std::string(message, length) << "]" << std::endl;

// } // WebSocketDispatcher::disconnection

// // ----------------------------------------------------------------------

// void WebSocketDispatcher::error(std::conditional<true, int, void *>::type /*user*/)
// {
//     std::cout << std::this_thread::get_id() << " WS FAILURE: Connection failed! Timeout?" << std::endl;

// } // WebSocketDispatcher::error

// // ----------------------------------------------------------------------


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

int main()
{
    constexpr const int PORT = 3001, PORT_SSL = 3000;

    try {
          // std::cout << "hardware_concurrency: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << std::this_thread::get_id() << " main thread" << std::endl;

        uWS::Hub hub;

        uWS::Group<uWS::SERVER>* group = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        HttpRequestRedirectToHttps redirect{group, PORT_SSL};
        if (!hub.listen(PORT, nullptr, 0, group))
            throw std::runtime_error("Listening http failed");

        uWS::Group<uWS::SERVER>* sslGroup = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        uS::TLS::Context tls_context = uS::TLS::createContext("/Users/eu/Desktop/AcmacsWeb.app/Contents/etc/self-signed.crt", "/Users/eu/Desktop/AcmacsWeb.app/Contents/etc/self-signed.key", "");
        if (!hub.listen(PORT_SSL, tls_context, uS::ListenOptions::REUSE_PORT, sslGroup))
            throw std::runtime_error("Listening https failed");
        HttpRequestDispatcher dispatcher{sslGroup};
        WebSocketHandlers<WebSocketData> handlers{sslGroup, 3 /* std::thread::hardware_concurrency() */};

        hub.run();
        return 0;
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << std::endl;
        return 1;
    }
}

// ----------------------------------------------------------------------



// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
