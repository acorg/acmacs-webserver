#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

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

int main()
{
    char hostname[1024];
    if (gethostname(hostname, sizeof hostname))
        strcpy(hostname, "localhost");
    std::cout << "hostname: " << hostname << std::endl;
    WsppHttp http{hostname, "3000"};
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
