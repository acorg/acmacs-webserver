#pragma once

#include <memory>

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
// both gcc and clnag
#pragma GCC diagnostic ignored "-Wunused-parameter"
#ifdef __clang__
//#pragma GCC diagnostic ignored ""
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wtautological-type-limit-compare"
#pragma GCC diagnostic ignored "-Wtautological-unsigned-enum-zero-compare"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"

// websocketpp has "using std::auto_ptr" not available in C++17
// simulate it
namespace std { template <typename T> using auto_ptr = unique_ptr<T>; }

#endif
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#pragma GCC diagnostic pop

#include "print.hh"

// ----------------------------------------------------------------------

using message_ptr = websocketpp::config::asio::message_type::ptr;

class WsppWebsocketLocationHandler;
class Wspp;
class WsppThread;

using WsppThreadMaker = std::function<WsppThread*(Wspp&)>;

// ----------------------------------------------------------------------

namespace _wspp_internal
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

        void data_available();
        void wait_for_data();

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

} // namespace _wspp_internal

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
