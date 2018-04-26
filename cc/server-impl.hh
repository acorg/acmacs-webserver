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
        inline QueueElement(std::shared_ptr<WsppWebsocketLocationHandler> aConnected, Handler aHandler, std::string aMessage) : connected{aConnected}, message{aMessage}, handler{aHandler} {}

        std::shared_ptr<WsppWebsocketLocationHandler> connected;
        std::string message;
        Handler handler;

          // inline void call(WsppWebsocketLocationHandler& aHandler) { (aHandler.*handler)(message); }

    }; // class QueueElement

      // ----------------------------------------------------------------------

    class Queue : public std::queue<QueueElement>
    {
     public:
        inline Queue() = default; // : std::queue<QueueElement>{} {}

        inline void push(std::shared_ptr<WsppWebsocketLocationHandler> aConnected, QueueElement::Handler aHandler, std::string aMessage = std::string{})
            {
                std::queue<QueueElement>::emplace(aConnected, aHandler, aMessage);
                // print_cerr(std::this_thread::get_id(), " queue::push after size: ", std::queue<QueueElement>::size());
                data_available();
            }

        inline QueueElement pop()
            {
                wait_for_data();
                // print_cerr(std::this_thread::get_id(), " queue::pop before size: ", std::queue<QueueElement>::size());
                auto result = std::queue<QueueElement>::front();
                std::queue<QueueElement>::pop();
                // print_cerr(std::this_thread::get_id(), " queue::pop after size: ", std::queue<QueueElement>::size());
                return result;
            }

     private:
        std::condition_variable mNotifier;
        std::mutex mMutexForNotifier;

        inline void data_available()
            {
                std::unique_lock<std::mutex> lock{mMutexForNotifier};
                mNotifier.notify_one();
            }

        inline void wait_for_data()
            {
                while (std::queue<QueueElement>::empty()) {
                    std::unique_lock<std::mutex> lock{mMutexForNotifier};
                    mNotifier.wait(lock, [this]() -> bool { return !this->empty(); });
                }
            }

    }; // Queue

      // ----------------------------------------------------------------------

    class Threads : public std::vector<std::shared_ptr<WsppThread>>
    {
      public:
        inline Threads(Wspp& aWspp, size_t aNumberOfThreads, WsppThreadMaker aThreadMaker)
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
          // inline ~WsppImplementation() { print_cerr(std::this_thread::get_id(), " ~WsppImplementation"); }

        inline void listen(std::string aHost, std::string aPort)
            {
                constexpr const size_t max_attempts = 30;
                for (size_t attempt = 1; ; ++attempt) {
                    try {
                        mServer.listen(aHost, aPort);
                        if (attempt > 1)
                            std::cerr << std::endl;
                        print_cerr("Listening at ", aHost, ':', aPort);
                        break;
                    }
                    catch (std::exception& err) {
                        if (attempt < max_attempts) {
                            using namespace std::chrono_literals;
                            if (attempt == 1)
                                std::cerr << "Cannot listen  at " << aHost << ':' << aPort << ": " << err.what() << ", retrying in 3s, attempt: " << attempt;
                            else
                                std::cerr << ' ' << attempt;
                            std::this_thread::sleep_for(3s);
                        }
                        else {
                            print_cerr("Cannot listen  at ", aHost, ':', aPort, ": ", err.what(), ", exiting after ", attempt);
                            exit(1);
                        }
                    }
                }
            }

        inline void send(websocketpp::connection_hdl hdl, std::string aMessage, websocketpp::frame::opcode::value op_code)
            {
                if (!hdl.expired())
                    mServer.send(hdl, aMessage, op_code);
            }

        inline auto& server() { return mServer; }
        inline auto connection(websocketpp::connection_hdl hdl) { return mServer.get_con_from_hdl(hdl); }
        inline auto& queue() { return mQueue; }
        inline void start_threads() { mThreads.start(); }
        inline void stop_listening() { mServer.stop_listening(); }

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

        inline void close(websocketpp::connection_hdl hdl, std::string aReason) { return mServer.close(hdl, websocketpp::close::status::unsupported_data, aReason); }

    }; // class WsppImplementation

      // ----------------------------------------------------------------------

} // namespace _wspp_internal

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
