#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <getopt.h>

#include "wspp-http.hh"

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
 protected:
    virtual inline bool use(std::string aLocation)
        {
            return aLocation == "/myws";
        }

    virtual inline void on_open(websocketpp::connection_hdl hdl)
        {
            send(hdl, "hello");
        }

    virtual inline void on_message(websocketpp::connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg)
        {
            std::cerr << "MSG (op-code: " << msg->get_opcode() << "): \"" << msg->get_payload() << '"' << std::endl;
            send(hdl, "MyWS first", websocketpp::frame::opcode::binary);
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

      // char hostname[1024];
      // if (gethostname(hostname, sizeof hostname))
      //     strcpy(hostname, "localhost");
    WsppHttp http{hostname, port};
    http.setup_logging("/tmp/wspp.access.log", "/tmp/wspp.error.log");
    http.add_location_handler(std::make_shared<RootPage>());
    http.add_location_handler(std::make_shared<WsppHttpLocationHandlerFile>("/f/myscript.js", std::vector<std::string>{"f/myscript.js", "f/myscript.js.gz"}));
    http.add_location_handler(std::make_shared<WsppHttpLocationHandlerFile>("/favicon.ico", std::vector<std::string>{"favicon.ico"}));
    http.add_location_handler(std::make_shared<MyWS>());

    http.run();
    return 0;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
