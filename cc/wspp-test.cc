#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

#include "wspp-http.hh"

// #include "wspp.hh"

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

}; // class WsppHttpLocationHandler404

// ----------------------------------------------------------------------

int main()
{
    WsppHttp http{"localhost", "3000"};
    http.add_http_location_handler(std::make_shared<RootPage>());
    http.add_http_location_handler(std::make_shared<WsppHttpLocationHandlerFile>("/f/myscript.js", std::vector<std::string>{"f/myscript.js", "f/myscript.js.gz"}));
    http.add_http_location_handler(std::make_shared<WsppHttpLocationHandlerFile>("/favicon.ico", std::vector<std::string>{"favicon.ico"}));

    http.run();
    return 0;
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

#if 0

using server = websocketpp::server<websocketpp::config::asio_tls>;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

// See https://wiki.mozilla.org/Security/Server_Side_TLS for more details about
// the TLS modes. The code below demonstrates how to implement both the modern
enum tls_mode {
    MOZILLA_INTERMEDIATE = 1,
    MOZILLA_MODERN = 2
};

void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg);
void on_open(server* s, websocketpp::connection_hdl hdl);
void on_http(server* s, websocketpp::connection_hdl hdl);
context_ptr on_tls_init(tls_mode mode, websocketpp::connection_hdl hdl);
void reply_404(server::connection_ptr con, std::string location);

void on_open(server* s, websocketpp::connection_hdl hdl)
{
    std::cout << "web socket connection opened" << std::endl;
    s->send(hdl, "hello", websocketpp::frame::opcode::text); // websocketpp::frame::opcode::binary
}

void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg)
{
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;

    try {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Echo failed because: " << e
                  << "(" << e.message() << ")" << std::endl;
    }
}

void on_http(server* s, websocketpp::connection_hdl hdl)
{
    server::connection_ptr con = s->get_con_from_hdl(hdl);
    const std::string location = con->get_resource();

    if (location == "/f/myscript.js") {
        std::string filename{location, 1};
        std::ifstream file{filename.c_str()};
        if (!file) {
            file.open((filename + ".gz").c_str());
            if (file)
                con->append_header("Content-Encoding", "gzip");
        }
        if (file) {
            std::string reply;
            file.seekg(0, std::ios::end);
            reply.reserve(static_cast<size_t>(file.tellg()));
            file.seekg(0, std::ios::beg);
            reply.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            con->set_body(reply);
            con->set_status(websocketpp::http::status_code::ok);
        }
        else {
            reply_404(con, location);
        }
    }
    else if (location == "/") {
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string reply = std::string{"<html><head><script src=\"/f/myscript.js\"></script></head><body><h1>UWS-TEST</h1><p>"} + std::asctime(std::localtime(&now)) + "</p></body></html>";
        con->set_body(reply);
        con->set_status(websocketpp::http::status_code::ok);
    }
    else {
        // s->get_alog().write(websocketpp::log::alevel::app, "Error 404: not found: " + location);
        reply_404(con, location);
    }
      // ~/AD/sources/websocketpp/websocketpp/connection.hpp
      //   // POST
      // std::string res = con->get_request_body();
      // std::cout << "got HTTP request with " << res.size() << " bytes of body data." << std::endl;
    // std::cout << "http secure: " << con->get_secure() << std::endl;
    // std::cout << "http host: \"" << con->get_host() << '"' << std::endl;
    // std::cout << "http port: " << con->get_port() << std::endl;
    // std::cout << "http resource: \"" << con->get_resource() << '"' << std::endl; // location in URL
      // std::cout << "http header: \"" << con->get_request_header("???") << '"' << std::endl;
    // std::cout << "http origin: \"" << con->get_origin() << '"' << std::endl; // SEG FAULT
}

void reply_404(server::connection_ptr con, std::string location)
{
    std::string reply = "<!doctype html><html><head><title>Error 404 (Resource not found)</title><body><h1>Error 404</h1><p>The requested URL " + location + " was not found on this server.</p></body></head></html>";
    con->set_body(reply);
    con->set_status(websocketpp::http::status_code::not_found);
}

context_ptr on_tls_init(tls_mode mode, websocketpp::connection_hdl /*hdl*/)
{
    namespace asio = websocketpp::lib::asio;

      // std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;

    context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

    try {
          // std::cout << "using TLS mode: " << (mode == MOZILLA_MODERN ? "Mozilla Modern" : "Mozilla Intermediate") << std::endl;
        if (mode == MOZILLA_MODERN) {
              // Modern disables TLSv1
            ctx->set_options(asio::ssl::context::default_workarounds |
                             asio::ssl::context::no_sslv2 |
                             asio::ssl::context::no_sslv3 |
                             asio::ssl::context::no_tlsv1 |
                             asio::ssl::context::single_dh_use);
        } else {
            ctx->set_options(asio::ssl::context::default_workarounds |
                             asio::ssl::context::no_sslv2 |
                             asio::ssl::context::no_sslv3 |
                             asio::ssl::context::single_dh_use);
        }

          // ctx->set_password_callback(websocketpp::lib::bind([]() { return std::string{}; }));
        ctx->use_certificate_chain_file("ssl/self-signed.crt");
        ctx->use_private_key_file("ssl/self-signed.key", asio::ssl::context::pem);
        ctx->use_tmp_dh_file("ssl/dh.pem");

        std::string ciphers;
        if (mode == MOZILLA_MODERN) {
            ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
        } else {
            ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
        }
        if (SSL_CTX_set_cipher_list(ctx->native_handle() , ciphers.c_str()) != 1) {
            std::cout << "Error setting cipher list" << std::endl;
        }

    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return ctx;
}

int main()
{
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    using websocketpp::lib::bind;

    server echo_server;

    echo_server.init_asio();

    echo_server.set_message_handler(bind(&on_message, &echo_server, _1, _2));
    echo_server.set_open_handler(bind(&on_open, &echo_server, _1));
    echo_server.set_http_handler(bind(&on_http, &echo_server, _1));
    echo_server.set_tls_init_handler(bind(&on_tls_init, MOZILLA_INTERMEDIATE, _1)); // all browsers on macOS want TLSv1 -> MOZILLA_INTERMEDIATE

    echo_server.listen("localhost", "3000");
    echo_server.start_accept();
    echo_server.run();

    return 0;

}

// ----------------------------------------------------------------------

    void on_http(connection_hdl hdl) {
        // Upgrade our connection handle to a full connection_ptr
        server::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);

        std::ifstream file;
        std::string filename = con->get_resource();
        std::string response;

        m_endpoint.get_alog().write(websocketpp::log::alevel::app,
            "http request1: "+filename);

        if (filename == "/") {
            filename = m_docroot+"index.html";
        } else {
            filename = m_docroot+filename.substr(1);
        }

        m_endpoint.get_alog().write(websocketpp::log::alevel::app,
            "http request2: "+filename);

        file.open(filename.c_str(), std::ios::in);
        if (!file) {
            // 404 error
            std::stringstream ss;

            ss << "<!doctype html><html><head>"
               << "<title>Error 404 (Resource not found)</title><body>"
               << "<h1>Error 404</h1>"
               << "<p>The requested URL " << filename << " was not found on this server.</p>"
               << "</body></head></html>";

            con->set_body(ss.str());
            con->set_status(websocketpp::http::status_code::not_found);
            return;
        }

        file.seekg(0, std::ios::end);
        response.reserve(file.tellg());
        file.seekg(0, std::ios::beg);

        response.assign((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

        con->set_body(response);
        con->set_status(websocketpp::http::status_code::ok);

    BOOST_CHECK_EQUAL(con->get_response_code(), status_code::ok);
    BOOST_CHECK_EQUAL(con->get_response_msg(), status_code::get_string(status_code::ok));
    }

#endif

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
