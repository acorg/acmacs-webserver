#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <thread>
#include <getopt.h>

#include "acmacs-base/json-writer.hh"
#include "server.hh"
#include "server-settings.hh"

// ----------------------------------------------------------------------

class RootPage : public WsppHttpLocationHandler
{
 public:
    bool handle(const HttpResource& aResource, WsppHttpResponseData& aResponse) override
        {

            bool handled = false;
            if (aResource.location() == "/") {
                std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                aResponse.body = R"(<html><head><script src="/ads/f/myscript.js"></script>)";
                aResponse.body += "<script>ARGV = " + json_writer::compact_json(aResource.argv()) + "</script>";
                aResponse.body += "</head><body><h1>WSPP-TEST RootPage</h1><p>";
                aResponse.body += std::asctime(std::localtime(&now));
                aResponse.body += "</p></body></html>";
                handled = true;
            }
            return handled;
        }

}; // class RootPage

// ----------------------------------------------------------------------

class MyWS : public WsppWebsocketLocationHandler
{
 public:
    MyWS() : WsppWebsocketLocationHandler{} {}
    MyWS(const MyWS& aSrc) : WsppWebsocketLocationHandler{aSrc} {}

 protected:
    std::shared_ptr<WsppWebsocketLocationHandler> clone() const override
        {
            return std::make_shared<MyWS>(*this);
        }

    bool use(std::string aLocation) const override
        {
            return aLocation == "/myws";
        }

    void opening(std::string, WsppThread& /*aThread*/) override
        {
            std::cerr << std::this_thread::get_id() << " MyWS opening" << std::endl;
            send("hello");
        }

    void message(std::string aMessage, WsppThread& /*aThread*/) override
        {
            std::cerr << std::this_thread::get_id() << " MyWS message: \"" << aMessage << '"' << std::endl;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(2s);
            std::cerr << std::this_thread::get_id() << " MyWS message processed, replying" << std::endl;
            send("MyWS first", websocketpp::frame::opcode::binary);
        }

    void after_close(std::string, WsppThread& /*aThread*/) override
        {
            std::cout << std::this_thread::get_id() << " MyWS after_close" << std::endl;
        }

}; // class MyWS

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    if (argc != 2 || std::string{"-h"} == argv[1] || std::string{"--help"} == argv[1]) {
        std::cerr << "Usage: " << argv[0] << " <settings.json>" << std::endl;
        return 1;
    }

    ServerSettings settings{argv[1]};
    Wspp wspp{settings, &WsppThread::make};

    wspp.add_location_handler(std::make_shared<RootPage>());
    wspp.add_location_handler(std::make_shared<MyWS>());

    wspp.run();

    return 0;
}

// int main(int argc, char* const argv[])
// {
//     std::string hostname{"localhost"}, port{"1169"};
//     const char* const short_opts = "x:p:h";
//     const option long_opts[] = {
//         {"host", required_argument, nullptr, 'x'},
//         {"port", required_argument, nullptr, 'p'},
//         {"help", no_argument, nullptr, 'h'},
//         {nullptr, no_argument, nullptr, 0}
//     };
//     int opt;
//     while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
//         switch (opt) {
//           case 'x':
//               hostname = optarg;
//               break;
//           case 'p':
//               port = optarg;
//               break;
//           case 0:
//               std::cout << "getopt_long 0" << std::endl;
//               break;
//           case 'h':
//               std::cerr << "Usage: " << argv[0] << " [--host|-x <hostname>] [--port|-p <port>]" << std::endl;
//               return 0;
//           default:
//               break;
//         }
//     }
//     argc -= optind;
//     argv += optind;

//     Wspp wspp{hostname, port, 3 /* std::thread::hardware_concurrency() */, "ssl/self-signed.crt", "ssl/self-signed.key", "ssl/dh.pem"};
//     wspp.setup_logging("/tmp/wspp.access.log", "/tmp/wspp.error.log");
//     wspp.add_location_handler(std::make_shared<RootPage>());
//     wspp.add_location_handler(std::make_shared<WsppHttpLocationHandlerFile>("/f/myscript.js", std::vector<std::string>{"f/myscript.js", "f/myscript.js.gz"}));
//     wspp.add_location_handler(std::make_shared<WsppHttpLocationHandlerFile>("/favicon.ico", std::vector<std::string>{"favicon.ico"}));
//     wspp.add_location_handler(std::make_shared<MyWS>());

//     wspp.run();
//     return 0;
// }

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
