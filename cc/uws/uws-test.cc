#include <string>
#include <iostream>
#include <chrono>

#include "http-request-dispatcher.hh"
#include "websocket-dispatcher.hh"

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
