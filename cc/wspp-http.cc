#include <iostream>
#include <fstream>

#include "wspp-http.hh"

#include "wspp.hh"

// ----------------------------------------------------------------------

using message_ptr = websocketpp::config::asio::message_type::ptr;
using context_ptr = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;

// See https://wiki.mozilla.org/Security/Server_Side_TLS for more details about
// the TLS modes. The code below demonstrates how to implement both the modern
enum tls_mode {
    MOZILLA_INTERMEDIATE = 1,
    MOZILLA_MODERN = 2
};

static context_ptr on_tls_init(WsppHttp* wspp_http, websocketpp::connection_hdl hdl);

// ----------------------------------------------------------------------

bool WsppHttpLocationHandler404::handle(std::string aLocation, WsppHttpResponseData& aResponse)
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    aResponse.body = std::string{"<html><head><script src=\"/f/myscript.js\"></script></head><body><h1>UWS-TEST</h1><p>"} + std::asctime(std::localtime(&now)) + "</p></body></html>";
    aResponse.status = websocketpp::http::status_code::not_found;
    return true;

} // WsppHttpLocationHandler404::handle

// ----------------------------------------------------------------------

WsppHttp::WsppHttp(std::string aHost, std::string aPort)
    : certificate_chain_file{"ssl/self-signed.crt"},
      private_key_file{"ssl/self-signed.key"},
      tmp_dh_file{"ssl/dh.pem"},
      mServer{new server}
{
    mServer->init_asio();

    using websocketpp::lib::bind;
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;

    mServer->set_tls_init_handler(bind(&on_tls_init, this, _1));
    mServer->set_http_handler(bind(&WsppHttp::on_http, this, mServer, _1));
    // mServer->set_message_handler(bind(&on_message, &echo_server, _1, _2));
    // mServer->set_open_handler(bind(&on_open, &echo_server, _1));

    mServer->listen(aHost, aPort);

} // WsppHttp::WsppHttp

// ----------------------------------------------------------------------

WsppHttp::~WsppHttp()
{
    delete mServer;

} // WsppHttp::~WsppHttp

// ----------------------------------------------------------------------

void WsppHttp::run()
{
    add_http_location_handler(std::make_shared<WsppHttpLocationHandler404>());
    mServer->start_accept();
    mServer->run();

} // WsppHttp::run

// ----------------------------------------------------------------------

void WsppHttp::on_http(server* s, websocketpp::connection_hdl hdl)
{
    server::connection_ptr con = s->get_con_from_hdl(hdl);
    const std::string location = con->get_resource();

    WsppHttpResponseData response;
    std::cerr << "Handlers: " << mHttpLocationHandlers.size() << std::endl;
    for (auto handler: mHttpLocationHandlers) {
        if (handler->handle(location, response))
            break;
    }
    for (const auto& header: response.headers)
        con->append_header(header.first, header.second);
    con->set_body(response.body);
    con->set_status(response.status);

    // if (location == "/f/myscript.js") {
    //     std::string filename{location, 1};
    //     std::ifstream file{filename.c_str()};
    //     if (!file) {
    //         file.open((filename + ".gz").c_str());
    //         if (file)
    //             con->append_header("Content-Encoding", "gzip");
    //     }
    //     if (file) {
    //         std::string reply;
    //         file.seekg(0, std::ios::end);
    //         reply.reserve(static_cast<size_t>(file.tellg()));
    //         file.seekg(0, std::ios::beg);
    //         reply.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    //         con->set_body(reply);
    //         con->set_status(websocketpp::http::status_code::ok);
    //     }
    //     else {
    //         reply_404(con, location);
    //     }
    // }
    // else if (location == "/") {
    //     std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    //     std::string reply = std::string{"<html><head><script src=\"/f/myscript.js\"></script></head><body><h1>UWS-TEST</h1><p>"} + std::asctime(std::localtime(&now)) + "</p></body></html>";
    //     con->set_body(reply);
    //     con->set_status(websocketpp::http::status_code::ok);
    // }
    // else {
    //     // s->get_alog().write(websocketpp::log::alevel::app, "Error 404: not found: " + location);
    //     reply_404(con, location);
    // }

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

} // WsppHttp::on_http

// ----------------------------------------------------------------------

context_ptr on_tls_init(WsppHttp* wspp_http, websocketpp::connection_hdl /*hdl*/)
{
    namespace asio = websocketpp::lib::asio;

    const tls_mode mode = MOZILLA_INTERMEDIATE; // all browsers on macOS want TLSv1 -> MOZILLA_INTERMEDIATE

    context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

    try {
          // std::cout << "using TLS mode: " << (mode == MOZILLA_MODERN ? "Mozilla Modern" : "Mozilla Intermediate") << std::endl;
        const char* ciphers = nullptr;
        switch (mode) {
          case MOZILLA_MODERN:
                // Modern disables TLSv1
              ctx->set_options(asio::ssl::context::default_workarounds |
                               asio::ssl::context::no_sslv2 |
                               asio::ssl::context::no_sslv3 |
                               asio::ssl::context::no_tlsv1 |
                               asio::ssl::context::single_dh_use);
              ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
              break;
          case MOZILLA_INTERMEDIATE:
              ctx->set_options(asio::ssl::context::default_workarounds |
                               asio::ssl::context::no_sslv2 |
                               asio::ssl::context::no_sslv3 |
                               asio::ssl::context::single_dh_use);
              ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
              break;
        }

          // ctx->set_password_callback(websocketpp::lib::bind([]() { return std::string{}; }));
        ctx->use_certificate_chain_file(wspp_http->certificate_chain_file);
        ctx->use_private_key_file(wspp_http->private_key_file, asio::ssl::context::pem);
        ctx->use_tmp_dh_file(wspp_http->tmp_dh_file);

        if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers) != 1) {
            std::cout << "Error setting cipher list" << std::endl;
        }

    }
    catch (std::exception& e) {
        std::cout << "Exception in initializing TLS: " << e.what() << std::endl;
        throw;
    }
    return ctx;

} // on_tls_init

// ----------------------------------------------------------------------

void WsppHttp::on_open(server* s, websocketpp::connection_hdl hdl)
{

} // WsppHttp::on_open

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
