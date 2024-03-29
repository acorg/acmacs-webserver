#pragma once

#include <memory>

#include "websocketpp-asio.hh"
#include "print.hh"

// ----------------------------------------------------------------------

using message_ptr = websocketpp::config::asio::message_type::ptr;

class WsppWebsocketLocationHandler;
class Wspp;
class WsppThread;

using WsppThreadMaker = std::function<WsppThread*(Wspp&)>;

// ----------------------------------------------------------------------

namespace wspp_internal
{

    class QueueElement
    {
     public:
        using Handler = void (WsppWebsocketLocationHandler::*)(std::string aMessage, WsppThread& aThread);
        QueueElement(std::shared_ptr<WsppWebsocketLocationHandler> aConnected, Handler aHandler, std::string aMessage) : connected{aConnected}, message{aMessage}, handler{aHandler} {}

        std::shared_ptr<WsppWebsocketLocationHandler> connected;
        std::string message;
        Handler handler;

          // void call(WsppWebsocketLocationHandler& aHandler) { (aHandler.*handler)(message); }

    }; // class QueueElement

      // ----------------------------------------------------------------------

    class Queue : public std::queue<QueueElement>
    {
     public:
        Queue() = default; // : std::queue<QueueElement>{} {}

        void push(std::shared_ptr<WsppWebsocketLocationHandler> aConnected, QueueElement::Handler aHandler, std::string aMessage = std::string{});
        QueueElement pop();

     private:
        std::condition_variable mNotifier;
        std::mutex mMutexForNotifier;
        std::mutex mMutexForPop;

          //void data_available();
        // void wait_for_data();

    }; // Queue

      // ----------------------------------------------------------------------

    class Threads : public std::vector<std::shared_ptr<WsppThread>>
    {
      public:
        Threads(Wspp& aWspp, size_t aNumberOfThreads, WsppThreadMaker aThreadMaker)
            : std::vector<std::shared_ptr<WsppThread>>{aNumberOfThreads > 0 ? aNumberOfThreads : 4}, mWspp{aWspp}, mThreadMaker{aThreadMaker}
            {
            }

        void start();

     private:
        Wspp& mWspp;
        WsppThreadMaker mThreadMaker;

    }; // class Threads

      // ----------------------------------------------------------------------

    class WsppImplementation
    {
     public:
        WsppImplementation(Wspp& aParent, size_t aNumberOfThreads, WsppThreadMaker aThreadMaker);
          // ~WsppImplementation() { print_cerr(std::this_thread::get_id(), " ~WsppImplementation"); }

        void listen(std::string aHost, std::string aPort);

        void send(websocketpp::connection_hdl hdl, std::string aMessage, websocketpp::frame::opcode::value op_code)
            {
                if (!hdl.expired())
                    mServer.send(hdl, aMessage, op_code);
            }

        auto& server() { return mServer; }
        auto connection(websocketpp::connection_hdl hdl) { return mServer.get_con_from_hdl(hdl); }
        auto& queue() { return mQueue; }
        void start_threads() { mThreads.start(); }
        void stop_listening() { mServer.stop_listening(); }

          // runs in the thread
        void pop_call(WsppThread& aThread);

     private:
        Wspp& mParent;
        websocketpp::server<websocketpp::config::asio_tls> mServer;
        Queue mQueue;
        Threads mThreads;

        using context_ptr = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;

        context_ptr on_tls_init(websocketpp::connection_hdl hdl);
        void on_http(websocketpp::connection_hdl hdl);
        void on_open(websocketpp::connection_hdl hdl);

        void close(websocketpp::connection_hdl hdl, std::string aReason) { return mServer.close(hdl, websocketpp::close::status::unsupported_data, aReason); }

    }; // class WsppImplementation

      // ----------------------------------------------------------------------

} // namespace wspp_internal

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
