#pragma once

#include <iostream>

#include "uws.hh"

// ----------------------------------------------------------------------

namespace _websocket_dispatcher_internal
{
    template <typename Data> class QueueData
    {
     public:
          // template <typename ... Args> inline QueueData(std::string aMessage, Args ... aArgs) : data{aArgs ...}, message{aMessage} {}
        inline QueueData(Data& aData, /* HandlerPtr aHandlerPtr, */ std::string aMessage) : data{aData}, /* mHandlerPtr{aHandlerPtr}, */ message{aMessage} {}
        Data& data;
          // HandlerPtr mHandlerPtr;
        std::string message;

    }; // class QueueData<>

    template <typename Data> using QueueBase = std::queue<QueueData<Data>>;

// ----------------------------------------------------------------------

    template <typename Data> class WebSocketQueue : public QueueBase<Data>
    {
     private:
        using Base = QueueBase<Data>;

     public:
        inline WebSocketQueue(uWS::Group<uWS::SERVER>* aGroup) : Base{}, mGroup{aGroup} {}

        inline auto& group() { return *mGroup; }

        inline void push(Data& aData, /* typename Data::HandlerPtr aHandlerPtr, */ std::string aMessage = std::string{})
            {
                Base::emplace(aData, /* aHandlerPtr, */ aMessage);
                std::cerr << std::this_thread::get_id() << " Q.push " << Base::size() << " " << aData << std::endl;
                data_available();
            }

        inline QueueData<Data> pop()
            {
                wait_for_data();
                auto result = Base::front();
                Base::pop();
                std::cerr << std::this_thread::get_id() << " Q.pop " << Base::size() << std::endl;
                return result;
            }

     private:
        uWS::Group<uWS::SERVER>* mGroup;
        std::condition_variable mNotifier;
        std::mutex mMutexForNotifier;

        inline void data_available()
            {
                std::unique_lock<std::mutex> lock{mMutexForNotifier};
                mNotifier.notify_one();
            }

        inline void wait_for_data()
            {
                while (Base::empty()) {
                    std::unique_lock<std::mutex> lock{mMutexForNotifier};
                    mNotifier.wait(lock, [this]() -> bool { return !this->empty(); });
                }
            }

    }; // WebSocketQueue<Data>

// ----------------------------------------------------------------------

    template <typename Data> class DataSet : std::vector<Data>
    {
        using Base = std::vector<Data>;

     public:
        inline DataSet() : Base{} {}

        inline Data& find(uWS::WebSocket<uWS::SERVER>* aWs)
            {
                std::unique_lock<std::mutex> lock{mMutex};
                auto found = std::find_if(Base::begin(), Base::end(), [&aWs](const auto& data) { return aWs == data.mWs; });
                if (found == Base::end()) {
                    Base::emplace_back(aWs);
                    found = Base::rbegin().base();
                }
                return *found;
            }

        inline void remove(const Data& aData)
            {
                std::unique_lock<std::mutex> lock{mMutex};
                erase(std::remove(Base::begin(), Base::end(), aData), Base::end());
            }

     private:
        std::mutex mMutex;

    }; // class DataSet<Data>

} // namespace _websocket_dispatcher_internal

// ----------------------------------------------------------------------

template <typename Data> class WebSocketDispatcher
{
 public:
    inline WebSocketDispatcher(uWS::Group<uWS::SERVER>* aGroup) : mQueue{aGroup} //, mHandlers{aHandlers}
        {
            using namespace std::placeholders;
            aGroup->onConnection(std::bind(&WebSocketDispatcher::connection, this, _1, _2));
            aGroup->onError(std::bind(&WebSocketDispatcher::error, this, _1));
            aGroup->onDisconnection(std::bind(&WebSocketDispatcher::disconnection, this, _1, _2, _3, _4));
            aGroup->onMessage(std::bind(&WebSocketDispatcher::message, this, _1, _2, _3, _4));
              // void onTransfer(std::function<void(WebSocket<isServer> *)> handler);
              // void onPing(std::function<void(WebSocket<isServer> *, char *, size_t)> handler);
              // void onPong(std::function<void(WebSocket<isServer> *, char *, size_t)> handler);
        }

 protected:
    inline void connection(uWS::WebSocket<uWS::SERVER>* ws, uWS::HttpRequest /*request*/)
        {
            std::cout << std::this_thread::get_id() << " WS " << ws << " dispatcher connection, pre queue: " << mQueue.size() << std::endl;
            mQueue.push(mDataSet.find(ws) /* , &WebSocketHandler<Data>::connection */);
            std::cout << std::this_thread::get_id() << " WS " << ws << " dispatcher connection, queue: " << mQueue.size() << std::endl;
        }

    inline void error(std::conditional<true, int, void *>::type /*user*/)
        {
            std::cerr << std::this_thread::get_id() << " WS FAILURE: Connection failed! Timeout?" << std::endl;
        }

    inline void disconnection(uWS::WebSocket<uWS::SERVER>* ws, int /*code*/, char* message, size_t length)
        {
            Data& data = mDataSet.find(ws);
              // immeditely tell about disconnection, ws will be destroyed upon returning from this function!
            data.websocket_disconnected();
            std::cout << std::this_thread::get_id() << " WS " << ws << " dispatcher disconnection, pre queue: " << mQueue.size() << std::endl;
            mQueue.push(data, /*&WebSocketHandler<Data>::disconnection,*/ {message, length});
        }

    inline void message(uWS::WebSocket<uWS::SERVER>* ws, char* message, size_t length, uWS::OpCode /*opCode*/)
        {
            mQueue.push(mDataSet.find(ws), /* &WebSocketHandler<Data>::message, */ {message, length});
        }

 private:
    _websocket_dispatcher_internal::WebSocketQueue<Data> mQueue;
    _websocket_dispatcher_internal::DataSet<Data> mDataSet;

}; // class WebSocketDispatcher<Data>

// ----------------------------------------------------------------------

// Base class for the Data template parameter in the above templates
class WebSocketDataBase
{
 public:
    inline WebSocketDataBase(uWS::WebSocket<uWS::SERVER>* aWs) : mWs(aWs) {}
    inline WebSocketDataBase(const WebSocketDataBase& wsd) : mMutex{/* cannot copy */}, mWs{wsd.mWs} {}
    inline virtual ~WebSocketDataBase() {}
      // inline void operator=(WebSocketDataBase&& aData) { std::unique_lock<std::mutex> lock{mMutex}; mWs = aData.mWs; }

 private:
    std::mutex mMutex;
    uWS::WebSocket<uWS::SERVER>* mWs;

    inline void websocket_disconnected() { std::unique_lock<std::mutex> lock{mMutex}; mWs = nullptr; }

    template <typename> friend class WebSocketDispatcher;
    template <typename> friend class _websocket_dispatcher_internal::DataSet;

    friend inline std::ostream& operator<<(std::ostream& out, const WebSocketDataBase& wsd) { return out << "WS:" << wsd.mWs << std::endl; }

}; // class WebSocketDataBase


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
