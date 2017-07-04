// #include <string>
// #include <iostream>
// #include <cerrno>
// #include <cstring>
// #include <sys/stat.h>

#include <chrono>

#include "http-request-dispatcher.hh"
#include "websocket-dispatcher.hh"


// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

// template <typename Data> class WebSocketHandlers;

// // runs in a thread
// template <typename Data> class WebSocketHandler
// {
//  private:
//     class DataAssigner
//     {
//      public:
//         inline DataAssigner(Data&& aData, Data& aHandlerData) : mHandlerData(aHandlerData) { mHandlerData = std::move(aData); }
//         inline ~DataAssigner() { mHandlerData.reset(); }
//      private:
//         Data& mHandlerData;
//     };

//  public:
//     inline WebSocketHandler(WebSocketHandlers<Data>& aHandlers) : mHandlers{aHandlers}, mThread{new std::thread(std::bind(&WebSocketHandler::run, this))} {}
//     inline ~WebSocketHandler() { std::cerr << std::this_thread::get_id() << " WebSocketHandler destruct!" << std::endl; /* kill thread? */ }

//     inline void connection(Data&& aData)
//         {
//             DataAssigner assigner{std::move(aData), mData};
//             std::cerr << std::this_thread::get_id() << " " << mData.websocket() << " connection" << std::endl;
//             send("hello");
//         }

//     inline void disconnection(Data&& aData)
//         {
//             std::cerr << std::this_thread::get_id() << " " << mData.websocket() << " disconnection" << std::endl;
//             aData.invalidate_websocket(); // aData.ws is invalid pointer! ws is already closed and most probably destroyed
//             DataAssigner assigner{std::move(aData), mData};
//         }

//     inline void message(Data&& aData)
//         {
//             DataAssigner assigner{std::move(aData), mData};
//             std::cerr << std::this_thread::get_id() << " " << mData.websocket() << " Message \"" << mData.received_message() << "\"" << std::endl;
//             using namespace std::chrono_literals;
//             std::this_thread::sleep_for(5s);
//             send(mData.received_message()); // echo
//         }

//     inline void websocket_disconnected(uWS::WebSocket<uWS::SERVER>* ws) { mData.websocket_disconnected(ws); }

//  private:
//     WebSocketHandlers<Data>& mHandlers;
//     std::thread* mThread;
//     Data mData;

//     [[noreturn]] inline void run()
//         {
//               // std::cerr << std::this_thread::get_id() << " run" << std::endl;
//             while (true) {
//                 mHandlers.pop().call_handler(*this); // pop() blocks waiting for the message from queue
//             }
//         }

//     inline void move_data(Data&& aData)
//         {
//             std::unique_lock<std::mutex> lock{mData.mMutex};
//             mData = std::move(aData);
//         }

//     inline void send(std::string aMessage, bool aBinary = false) { mData.send(aMessage, aBinary); }
//     inline void send(const char* aMessage) { mData.send(aMessage); }

// }; // WebSocketHandler

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

// template <typename Data> class WebSocketHandlers;

// // Receives requests from web socket and adds correposning task to the queuq
// template <typename Data> class WebSocketDispatcher
// {
//  public:
//     inline WebSocketDispatcher(WebSocketQueue<Data>& aQueue, WebSocketHandlers<Data>& aHandlers) : mQueue{aQueue}, mHandlers{aHandlers}
//         {
//             using namespace std::placeholders;
//             auto& group = aQueue.group();
//             group.onConnection(std::bind(&WebSocketDispatcher::connection, this, _1, _2));
//             group.onError(std::bind(&WebSocketDispatcher::error, this, _1));
//             group.onDisconnection(std::bind(&WebSocketDispatcher::disconnection, this, _1, _2, _3, _4));
//             group.onMessage(std::bind(&WebSocketDispatcher::message, this, _1, _2, _3, _4));
//               // void onTransfer(std::function<void(WebSocket<isServer> *)> handler);
//               // void onPing(std::function<void(WebSocket<isServer> *, char *, size_t)> handler);
//               // void onPong(std::function<void(WebSocket<isServer> *, char *, size_t)> handler);
//         }

//  protected:
//     inline void connection(uWS::WebSocket<uWS::SERVER>* ws, uWS::HttpRequest /*request*/)
//         {
//             std::cerr << std::this_thread::get_id() << " WS " << ws << " dispatcher connection, pre queue: " << mQueue.size() << std::endl;
//             mQueue.push(ws, &WebSocketHandler<Data>::connection);
//             std::cerr << std::this_thread::get_id() << " WS " << ws << " dispatcher connection, queue: " << mQueue.size() << std::endl;
//         }

//     inline void error(std::conditional<true, int, void *>::type /*user*/)
//         {
//             std::cerr << std::this_thread::get_id() << " WS FAILURE: Connection failed! Timeout?" << std::endl;
//         }

//     inline void disconnection(uWS::WebSocket<uWS::SERVER>* ws, int /*code*/, char *message, size_t length)
//         {
//               // immeditely tell handlers about disconnection, ws will be destroyed upon returning from this function!
//             mHandlers.disconnection(ws);
//             std::cerr << std::this_thread::get_id() << " WS " << ws << " dispatcher disconnection, pre queue: " << mQueue.size() << std::endl;
//             mQueue.push(ws, &WebSocketHandler<Data>::disconnection, {message, length});
//         }

//     inline void message(uWS::WebSocket<uWS::SERVER>* ws, char *message, size_t length, uWS::OpCode /*opCode*/)
//         {
//             mQueue.push(ws, &WebSocketHandler<Data>::message, {message, length});
//         }

//  private:
//     WebSocketQueue<Data>& mQueue;
//     WebSocketHandlers<Data>& mHandlers;
// };

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

// // Singletone, runs in the main thread
// template <typename Data> class WebSocketHandlers
// {
//  public:
//     inline WebSocketHandlers(uWS::Group<uWS::SERVER>* aGroup, size_t aNumberOfThreads = std::thread::hardware_concurrency()) // hardware_concurrency may return 0)
//         : mQueue{aGroup}, mDispatcher{mQueue, *this}, mHandlers{aNumberOfThreads > 0 ? aNumberOfThreads : 4}
//         {
//             std::transform(mHandlers.begin(), mHandlers.end(), mHandlers.begin(), [this](const auto&) { return std::make_shared<WebSocketHandler<Data>>(*this); });
//         }

//     inline Data pop() { return mQueue.pop(); }

//     inline void disconnection(uWS::WebSocket<uWS::SERVER>* ws)
//         {
//             std::for_each(mHandlers.begin(), mHandlers.end(), [&ws](auto& handler) { handler->websocket_disconnected(ws); });
//         }

//  private:
//     WebSocketQueue<Data> mQueue;
//     WebSocketDispatcher<Data> mDispatcher;
//     std::vector<std::shared_ptr<WebSocketHandler<Data>>> mHandlers;

// }; // class WebSocketHandlers

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

// class WebSocketData
// {
//  public:
//     using HandlerPtr = void (WebSocketHandler<WebSocketData>::*)(WebSocketData&& aData);

//     inline WebSocketData() : mWs(nullptr), mHandler{nullptr} {}
//     inline WebSocketData(uWS::WebSocket<uWS::SERVER>* aWs, HandlerPtr aHandler, std::string aMessage) : mWs(aWs), mHandler{aHandler}, mReceivedMessage{aMessage} {}
//     inline WebSocketData(const WebSocketData& wsd) : mMutex{/* cannot copy */}, mWs{wsd.mWs}, mHandler{wsd.mHandler}, mReceivedMessage{wsd.mReceivedMessage} {}
//     inline virtual ~WebSocketData() {}
//     inline void operator=(WebSocketData&& aData) { std::unique_lock<std::mutex> lock{mMutex}; mWs = aData.mWs; mHandler = aData.mHandler; mReceivedMessage = aData.mReceivedMessage; }
//     inline void reset() { std::unique_lock<std::mutex> lock{mMutex}; mWs = nullptr; mHandler = nullptr; mReceivedMessage.clear(); }

//     inline virtual void send(std::string aMessage, bool aBinary = false)
//         {
//             std::unique_lock<std::mutex> lock{mMutex};
//             if (mWs)             // can be set to nullptr to mark it disconnected
//                 mWs->send(aMessage.c_str(), aMessage.size(), aBinary ? uWS::BINARY : uWS::TEXT);
//         }

//     inline virtual void send(const char* aMessage)
//         {
//             std::unique_lock<std::mutex> lock{mMutex};
//             if (mWs)
//                 mWs->send(aMessage, strlen(aMessage), uWS::TEXT);
//         }

//     inline bool match(const WebSocketData& aData) const { return mWs == aData.mWs; }

//     inline void invalidate_websocket() { mWs = nullptr; }

//     inline void websocket_disconnected(uWS::WebSocket<uWS::SERVER>* aWs)
//         {
//             std::unique_lock<std::mutex> lock{mMutex};
//             if (mWs == aWs) {
//                 std::cerr << std::this_thread::get_id() << " WS " << mWs << " disconnected, nullify" << std::endl;
//                 invalidate_websocket();
//             }
//         }

//     inline auto websocket() { return mWs; }

//     inline const std::string& received_message() const { return mReceivedMessage; }

//  private:
//     std::mutex mMutex;
//     uWS::WebSocket<uWS::SERVER>* mWs;
//     HandlerPtr mHandler;
//     std::string mReceivedMessage;

//     inline void call_handler(WebSocketHandler<WebSocketData>& aHandler)
//         {
//             (aHandler.*mHandler)(std::move(*this));
//         }

//     friend class WebSocketHandler<WebSocketData>;

// }; // class WebSocketData

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

// // void WebSocketDispatcher::disconnection_client(uWS::WebSocket<uWS::CLIENT>* ws, int code, char* message, size_t length)
// // {
// //     std::cerr << "WS client disconnection: " << code << " [" << std::string(message, length) << "]" << std::endl;

// // } // WebSocketDispatcher::disconnection_client

// // ----------------------------------------------------------------------

// void WebSocketDispatcher::disconnection(uWS::WebSocket<uWS::SERVER>* ws, int code, char* message, size_t length)
// {
//     std::cerr << "WS server disconnection: " << code << " [" << std::string(message, length) << "]" << std::endl;

// } // WebSocketDispatcher::disconnection

// // ----------------------------------------------------------------------

// void WebSocketDispatcher::error(std::conditional<true, int, void *>::type /*user*/)
// {
//     std::cerr << std::this_thread::get_id() << " WS FAILURE: Connection failed! Timeout?" << std::endl;

// } // WebSocketDispatcher::error

// // ----------------------------------------------------------------------


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

class WebSocketData : public WebSocketDataBase
{
 public:
    inline WebSocketData(uWS::WebSocket<uWS::SERVER>* aWs) : WebSocketDataBase(aWs) {}

    virtual inline ~WebSocketData()
        {
            std::cerr << std::this_thread::get_id() << " " << data_id() << " DESTRUCTOR" << std::endl;
        }

    virtual inline void connection()
        {
            std::cerr << std::this_thread::get_id() << " " << data_id() << " connection" << std::endl;
            send("hello");
        }

    virtual inline void disconnection(std::string aMessage)
        {
            std::cerr << std::this_thread::get_id() << " " << data_id() << " disconnection \"" << aMessage << "\"" << std::endl;
        }

    virtual inline void message(std::string aMessage)
        {
            std::cerr << std::this_thread::get_id() << " " << data_id() << " Message \"" << aMessage << "\"" << std::endl;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(2s);
            std::cerr << std::this_thread::get_id() << " " << data_id() << " Message processed, replying" << std::endl;
            send(aMessage); // echo
        }

}; // class WebSocketData
// ----------------------------------------------------------------------

int main()
{
    constexpr const int PORT = 3001, PORT_SSL = 3000;

    try {
          // std::cerr << "hardware_concurrency: " << std::thread::hardware_concurrency() << std::endl;
        std::cerr << std::this_thread::get_id() << " main thread" << std::endl;

        uWS::Hub hub;

        uWS::Group<uWS::SERVER>* group = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        HttpRequestRedirectToHttps redirect{group, PORT_SSL};
        if (!hub.listen(PORT, nullptr, 0, group))
            throw std::runtime_error("Listening http failed");

        uWS::Group<uWS::SERVER>* sslGroup = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        uS::TLS::Context tls_context = uS::TLS::createContext("/Users/eu/Desktop/AcmacsWeb.app/Contents/etc/self-signed.crt", "/Users/eu/Desktop/AcmacsWeb.app/Contents/etc/self-signed.key", "");
        if (!hub.listen(PORT_SSL, tls_context, uS::ListenOptions::REUSE_PORT, sslGroup))
            throw std::runtime_error("Listening https failed");
        HttpRequestDispatcher http_dispatcher{sslGroup};
        // WebSocketHandlers<WebSocketData> handlers{sslGroup, 3 /* std::thread::hardware_concurrency() */};
        WebSocketDispatcher<WebSocketData> ws_dispatcher{sslGroup, 3 /* std::thread::hardware_concurrency() */};

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
