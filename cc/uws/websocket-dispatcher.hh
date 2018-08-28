#pragma once

#include <iostream>
#include <thread>
#include <condition_variable>
#include <memory>
#include <queue>
#include <vector>
#include <map>

#include "acmacs-base/to-string.hh"
#include "uws.hh"

// ----------------------------------------------------------------------

template <typename Data> class WebSocketDispatcher;

// ----------------------------------------------------------------------

namespace _websocket_dispatcher_internal
{
    template <typename Data> class WebSocketWorker;
    template <typename Data> using HandlerPtr = void (WebSocketWorker<Data>::*)(std::shared_ptr<Data> aData, std::string aMessage);

    template <typename Data> class QueueData
    {
     public:
        inline QueueData(std::shared_ptr<Data> aData, HandlerPtr<Data> aHandlerPtr, std::string aHandlerId, std::string aMessage) : data{aData}, mHandlerPtr{aHandlerPtr}, mHandlerId{aHandlerId}, message{aMessage} {}
        std::shared_ptr<Data> data;
        HandlerPtr<Data> mHandlerPtr;
        std::string mHandlerId;
        std::string message;

        inline void call(WebSocketWorker<Data>& aWorker) { (aWorker.*mHandlerPtr)(data, message); }

    }; // class QueueData<>

// ----------------------------------------------------------------------

    template <typename Data> using QueueBase = std::queue<QueueData<Data>>;

    template <typename Data> class WebSocketQueue : public QueueBase<Data>
    {
     private:
        using Base = QueueBase<Data>;

     public:
        inline WebSocketQueue(uWS::Group<uWS::SERVER>* aGroup) : Base{}, mGroup{aGroup} {}

        inline auto& group() { return *mGroup; }

        inline void push(std::shared_ptr<Data> aData, HandlerPtr<Data> aHandlerPtr, std::string aHandlerId, std::string aMessage = std::string{})
            {
                Base::emplace(aData, aHandlerPtr, aHandlerId, aMessage);
                data_available();
            }

        inline QueueData<Data> pop()
            {
                wait_for_data();
                auto result = Base::front();
                Base::pop();
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

      // cannot use vector here because it can be relocated during
      // adding new elements breaking references to elemnets for long
      // running message processing tasks.
    template <typename Data> using DataSetBase = std::map<uWS::WebSocket<uWS::SERVER>*, std::shared_ptr<Data>>;

    template <typename Data> class DataSet : DataSetBase<Data>
    {
        using Base = DataSetBase<Data>;

     public:
        inline DataSet() : Base{} {}

        inline std::shared_ptr<Data> find(uWS::WebSocket<uWS::SERVER>* aWs)
            {
                std::unique_lock<std::mutex> lock{mMutex};
                auto found = Base::find(aWs);
                if (found == Base::end())
                    found = Base::emplace(aWs, std::make_shared<Data>(aWs)).first;
                return found->second;
            }

        inline void remove(std::shared_ptr<Data> aData)
            {
                  // aData.mWs was nulled before, we cannot look up using it
                for (auto it = Base::begin(); it != Base::end(); ++it) {
                    if (aData == it->second) {
                        Base::erase(it);
                        break;
                    }
                }
            }

     private:
        std::mutex mMutex;

    }; // class DataSet<Data>

// ----------------------------------------------------------------------

    template <typename Data> class WebSocketWorker
    {
     public:
        inline WebSocketWorker(WebSocketDispatcher<Data>& aDispatcher) : mDispatcher{aDispatcher}, mThread{new std::thread(std::bind(&WebSocketWorker::run, this))}
            {
            }

        inline void connection(std::shared_ptr<Data> aData, std::string)
            {
                aData->connection();
            }

        inline void disconnection(std::shared_ptr<Data> aData, std::string aMessage)
            {
                aData->disconnection(aMessage);
            }

        inline void message(std::shared_ptr<Data> aData, std::string aMessage)
            {
                aData->message(aMessage);
            }

     private:
        WebSocketDispatcher<Data>& mDispatcher;
        std::thread* mThread;

        [[noreturn]] void run();

    }; // WebSocketWorker<Data>

// ----------------------------------------------------------------------

    template <typename Data> using WebSocketWorkersBase = std::vector<std::shared_ptr<WebSocketWorker<Data>>>;

    template <typename Data> class WebSocketWorkers : public WebSocketWorkersBase<Data>
    {
     private:
        using Base = WebSocketWorkersBase<Data>;

     public:
        inline WebSocketWorkers(WebSocketDispatcher<Data>& aDispatcher, size_t aNumberOfThreads)
            : Base{aNumberOfThreads > 0 ? aNumberOfThreads : 4}
            {
                std::transform(this->begin(), this->end(), this->begin(), [&aDispatcher](const auto&) { return std::make_shared<WebSocketWorker<Data>>(aDispatcher); });
            }

    }; // WebSocketWorkers<Worker>

} // namespace _websocket_dispatcher_internal

// ----------------------------------------------------------------------

template <typename Data> class WebSocketDispatcher
{
 public:
    inline WebSocketDispatcher(uWS::Group<uWS::SERVER>* aGroup, size_t aNumberOfThreads = std::thread::hardware_concurrency()) : mQueue{aGroup}, mWorkers{*this, aNumberOfThreads}
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
            mQueue.push(mDataSet.find(ws), &_websocket_dispatcher_internal::WebSocketWorker<Data>::connection, "connection");
        }

    inline void error(std::conditional<true, int, void *>::type /*user*/)
        {
            std::cerr << std::this_thread::get_id() << " WS FAILURE: Connection failed! Timeout?" << std::endl;
        }

    inline void disconnection(uWS::WebSocket<uWS::SERVER>* ws, int /*code*/, char* message, size_t length)
        {
            auto data = mDataSet.find(ws);
            mDataSet.remove(data);
              // immeditely tell about disconnection, ws will be destroyed upon returning from this function!
            data->websocket_disconnected();
            mQueue.push(data, &_websocket_dispatcher_internal::WebSocketWorker<Data>::disconnection, "disconnection", {message, length});
        }

    inline void message(uWS::WebSocket<uWS::SERVER>* ws, char* message, size_t length, uWS::OpCode /*opCode*/)
        {
            mQueue.push(mDataSet.find(ws), &_websocket_dispatcher_internal::WebSocketWorker<Data>::message, "message", {message, length});
        }

 private:
    _websocket_dispatcher_internal::WebSocketQueue<Data> mQueue;
    _websocket_dispatcher_internal::DataSet<Data> mDataSet;
    _websocket_dispatcher_internal::WebSocketWorkers<Data> mWorkers;

      // runs in the thread
    inline void pop_call(_websocket_dispatcher_internal::WebSocketWorker<Data>& worker)
        {
            auto data = mQueue.pop(); // blocks on waiting for a data in the queue
            data.call(worker);
        }

    // inline void destroy(Data& aData)
    //     {
    //         mDataSet.remove(aData);
    //     }

    friend class _websocket_dispatcher_internal::WebSocketWorker<Data>;

}; // class WebSocketDispatcher<Data>

// ----------------------------------------------------------------------

template <typename Data> [[noreturn]] inline void _websocket_dispatcher_internal::WebSocketWorker<Data>::run()
{
    while (true) {
        mDispatcher.pop_call(*this); // pop() blocks waiting for the message from queue
    }

} // _websocket_dispatcher_internal::WebSocketWorker::run

// ----------------------------------------------------------------------

// Base class for the Data template parameter in the above templates
class WebSocketDataBase
{
 public:
    inline WebSocketDataBase(uWS::WebSocket<uWS::SERVER>* aWs) : mWs(aWs), mId(++sId) {}
    inline virtual ~WebSocketDataBase() {}

      // The functions below are called in a thread to process event received via websocket.
      // They are must be re-entrant, to access data fields in this they MUST use std::unique_lock<std::mutex> lock{mMutex}!!!
    virtual void connection() = 0;
    virtual void disconnection(std::string aMessage) = 0; // mWs is null when this is called
    virtual void message(std::string aMessage) = 0;

    inline std::string data_id() const { /* std::unique_lock<std::mutex> lock{mMutex}; */ return std::to_string(mId) + ":" + acmacs::to_hex_string(mWs); }

 protected:
    mutable std::mutex mMutex;

    inline void send(std::string aMessage, bool aBinary = false)
        {
            std::unique_lock<std::mutex> lock{mMutex};
            if (mWs)             // can be set to nullptr to mark it disconnected
                mWs->send(aMessage.c_str(), aMessage.size(), aBinary ? uWS::BINARY : uWS::TEXT);
        }

    inline void send(const char* aMessage)
        {
            std::unique_lock<std::mutex> lock{mMutex};
            if (mWs)
                mWs->send(aMessage, strlen(aMessage), uWS::TEXT);
        }

 private:
    uWS::WebSocket<uWS::SERVER>* mWs;
    size_t mId;

    static size_t sId;

    [[noreturn]] inline WebSocketDataBase(const WebSocketDataBase&) { throw std::runtime_error("No copy constructor for WebSocketDataBase"); }
    [[noreturn]] inline void operator=(WebSocketDataBase&&) { throw std::runtime_error("No assignment for WebSocketDataBase"); }

    inline void websocket_disconnected()
        {
            std::unique_lock<std::mutex> lock{mMutex};
            mWs = nullptr;
        }

    template <typename> friend class WebSocketDispatcher;
    template <typename> friend class _websocket_dispatcher_internal::WebSocketWorker;

    // friend inline std::ostream& operator<<(std::ostream& out, const WebSocketDataBase& wsd) { return out << "WS:" << wsd.mWs << std::endl; }

}; // class WebSocketDataBase

size_t WebSocketDataBase::sId = 1;

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
