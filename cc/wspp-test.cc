#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <thread>
#include <getopt.h>

#include "server.hh"

// ----------------------------------------------------------------------

class RootPage : public WsppHttpLocationHandler
{
 public:
    virtual inline bool handle(std::string aLocation, WsppHttpResponseData& aResponse)
        {

            bool handled = false;
            if (aLocation == "/") {
                std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                aResponse.body = std::string{"<html><head><script src=\"/f/myscript.js\"></script></head><body><h1>WSPP-TEST RootPage</h1><p>"} + std::asctime(std::localtime(&now)) + "</p></body></html>";
                handled = true;
            }
            return handled;
        }

}; // class RootPage

// ----------------------------------------------------------------------

class MyWS : public WsppWebsocketLocationHandler
{
 public:
    inline MyWS() : WsppWebsocketLocationHandler{} {}
    inline MyWS(const MyWS& aSrc) : WsppWebsocketLocationHandler{aSrc} {}

 protected:
    virtual std::shared_ptr<WsppWebsocketLocationHandler> clone() const
        {
            return std::make_shared<MyWS>(*this);
        }

    virtual inline bool use(std::string aLocation) const
        {
            return aLocation == "/myws";
        }

    virtual inline void opening(std::string)
        {
            std::cerr << std::this_thread::get_id() << " MyWS opening" << std::endl;
            send("hello");
        }

    virtual inline void message(std::string aMessage)
        {
            std::cerr << std::this_thread::get_id() << " MyWS message: \"" << aMessage << '"' << std::endl;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(2s);
            std::cerr << std::this_thread::get_id() << " MyWS message processed, replying" << std::endl;
            send("MyWS first", websocketpp::frame::opcode::binary);
        }

    virtual void after_close(std::string)
        {
            std::cout << std::this_thread::get_id() << " MyWS after_close" << std::endl;
        }

}; // class MyWS

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    std::string hostname{"localhost"}, port{"1169"};
    const char* const short_opts = "x:p:h";
    const option long_opts[] = {
        {"host", required_argument, nullptr, 'x'},
        {"port", required_argument, nullptr, 'p'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, no_argument, nullptr, 0}
    };
    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
          case 'x':
              hostname = optarg;
              break;
          case 'p':
              port = optarg;
              break;
          case 0:
              std::cout << "getopt_long 0" << std::endl;
              break;
          case 'h':
              std::cerr << "Usage: " << argv[0] << " [--host|-x <hostname>] [--port|-p <port>]" << std::endl;
              return 0;
          default:
              break;
        }
    }
    argc -= optind;
    argv += optind;

    Wspp wspp{hostname, port, 3 /* std::thread::hardware_concurrency() */};
    wspp.setup_logging("/tmp/wspp.access.log", "/tmp/wspp.error.log");
    wspp.add_location_handler(std::make_shared<RootPage>());
    wspp.add_location_handler(std::make_shared<WsppHttpLocationHandlerFile>("/f/myscript.js", std::vector<std::string>{"f/myscript.js", "f/myscript.js.gz"}));
    wspp.add_location_handler(std::make_shared<WsppHttpLocationHandlerFile>("/favicon.ico", std::vector<std::string>{"favicon.ico"}));
    wspp.add_location_handler(std::make_shared<MyWS>());

    wspp.run();
    return 0;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
