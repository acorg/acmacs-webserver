#include <iostream>
#include <fstream>

#include "wspp-http.hh"

#include "wspp.hh"

// ----------------------------------------------------------------------

using message_ptr = websocketpp::config::asio::message_type::ptr;

// ----------------------------------------------------------------------

class WsppImplementation
{
 public:
    WsppImplementation(Wspp& aParent);

    inline void listen(std::string aHost, std::string aPort)
        {
            std::cout << "Listening at " << aHost << ':' << aPort << std::endl;
            mServer.listen(aHost, aPort);
        }

    inline void send(websocketpp::connection_hdl hdl, std::string aMessage, websocketpp::frame::opcode::value op_code)
        {
            mServer.send(hdl, aMessage, op_code);
        }

    inline auto& server() { return mServer; }
    inline auto connection(websocketpp::connection_hdl hdl) { return mServer.get_con_from_hdl(hdl); }

 private:
    Wspp& mParent;
    websocketpp::server<websocketpp::config::asio_tls> mServer;

    using context_ptr = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;

    context_ptr on_tls_init(websocketpp::connection_hdl hdl);
    void on_http(websocketpp::connection_hdl hdl);
    void on_open(websocketpp::connection_hdl hdl);
};

// ----------------------------------------------------------------------

WsppImplementation::WsppImplementation(Wspp& aParent)
    : mParent{aParent}
{
    mServer.init_asio();

    using websocketpp::lib::bind;
    using websocketpp::lib::placeholders::_1;

    mServer.set_tls_init_handler(bind(&WsppImplementation::on_tls_init, this, _1));
    mServer.set_http_handler(bind(&WsppImplementation::on_http, this, _1));
    mServer.set_open_handler(bind(&WsppImplementation::on_open, this, _1));

} // WsppImplementation::WsppImplementation

// ----------------------------------------------------------------------

WsppImplementation::context_ptr WsppImplementation::on_tls_init(websocketpp::connection_hdl /*hdl*/)
{
      // See https://wiki.mozilla.org/Security/Server_Side_TLS for more details about
      // the TLS modes. The code below demonstrates how to implement both the modern
    enum tls_mode {
        MOZILLA_INTERMEDIATE = 1,
        MOZILLA_MODERN = 2
    };

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
        ctx->use_certificate_chain_file(mParent.certificate_chain_file);
        ctx->use_private_key_file(mParent.private_key_file, asio::ssl::context::pem);
        ctx->use_tmp_dh_file(mParent.tmp_dh_file);

        if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers) != 1) {
            std::cout << "Error setting cipher list" << std::endl;
        }

    }
    catch (std::exception& e) {
        std::cout << "Exception in initializing TLS: " << e.what() << std::endl;
        throw;
    }
    return ctx;

} // WsppImplementation::on_tls_init

// ----------------------------------------------------------------------

void WsppImplementation::on_http(websocketpp::connection_hdl hdl)
{
    auto connection = mServer.get_con_from_hdl(hdl);
    const std::string location = connection->get_resource();

    WsppHttpResponseData response;
    mParent.http_location_handle(location, response);
    for (const auto& header: response.headers)
        connection->append_header(header.first, header.second);
    connection->set_body(response.body);
    connection->set_status(response.status);

      //   // POST
      // std::string res = con->get_request_body();
      // std::cout << "got HTTP request with " << res.size() << " bytes of body data." << std::endl;
    // std::cout << "http secure: " << con->get_secure() << std::endl;
    // std::cout << "http host: \"" << con->get_host() << '"' << std::endl;
    // std::cout << "http port: " << con->get_port() << std::endl;
    // std::cout << "http resource: \"" << con->get_resource() << '"' << std::endl; // location in URL
      // std::cout << "http header: \"" << con->get_request_header("???") << '"' << std::endl;
    // std::cout << "http origin: \"" << con->get_origin() << '"' << std::endl; // SEG FAULT

} // WsppImplementation::on_http

// ----------------------------------------------------------------------

void WsppImplementation::on_open(websocketpp::connection_hdl hdl)
{
    auto connected = mParent.find_create_connected(hdl);
    connected->on_open();

} // WsppImplementation::on_open

// ----------------------------------------------------------------------

Wspp::Wspp(std::string aHost, std::string aPort)
    : impl{new WsppImplementation{*this}},
      certificate_chain_file{"ssl/self-signed.crt"},
      private_key_file{"ssl/self-signed.key"},
      tmp_dh_file{"ssl/dh.pem"}
{
    impl->listen(aHost, aPort);

} // Wspp::Wspp

// ----------------------------------------------------------------------

Wspp::~Wspp()
{
} // Wspp::~Wspp

// ----------------------------------------------------------------------

void Wspp::setup_logging(std::string access_log_filename, std::string error_log_filename)
{
    using namespace websocketpp::log;
    auto& alog = impl->server().get_alog();
    if (!access_log_filename.empty()) {
        auto* fs = new std::ofstream{access_log_filename, std::ios_base::out | std::ios_base::app};
        if (fs && *fs)
            alog.set_ostream(fs);
        else
            delete fs;
    }

     // remove all logging, then enable individual channels

    alog.clear_channels(alevel::all);
    alog.set_channels(alevel::none // ~/AD/include/websocketpp/logger/levels.hpp
                      | alevel::connect
                      | alevel::disconnect
                        // | alevel::control
                        // | alevel::frame_header
                        // | alevel::frame_payload
                        // | alevel::message_header // Reserved
                        // | alevel::message_payload // Reserved
                        // | alevel::endpoint // Reserved
                        // | alevel::debug_handshake
                        // | alevel::debug_close
                        // | alevel::devel
                      | alevel::app // Special channel for application specific logs. Not used by the library.
                      | alevel::http // Access related to HTTP requests
                      | alevel::fail
                      ); // alevel::connect | alevel::disconnect);

    auto& elog = impl->server().get_elog();
    if (!error_log_filename.empty()) {
        auto* fs = new std::ofstream{error_log_filename, std::ios_base::out | std::ios_base::app};
        if (fs && *fs)
            elog.set_ostream(fs);
        else
            delete fs;
    }
    elog.clear_channels(alevel::all);
    elog.set_channels(alevel::none // ~/AD/include/websocketpp/logger/levels.hpp
                      | alevel::connect
                      | alevel::disconnect
                      | alevel::control
                      | alevel::frame_header
                      | alevel::frame_payload
                      | alevel::message_header
                      | alevel::message_payload
                      | alevel::endpoint
                      | alevel::debug_handshake
                      | alevel::debug_close
                      | alevel::devel
                      | alevel::app
                      | alevel::http // Access related to HTTP requests
                      | alevel::fail
                      ); // alevel::connect | alevel::disconnect);

} // Wspp::setup_logging

// ----------------------------------------------------------------------

void Wspp::run()
{
    add_location_handler(std::make_shared<WsppHttpLocationHandler404>());
      // $$ add default websocket location handler
    impl->server().start_accept();
    impl->server().run();

} // Wspp::run

// ----------------------------------------------------------------------

void Wspp::http_location_handle(std::string aLocation, WsppHttpResponseData& aResponse)
{
    for (auto handler: mHttpLocationHandlers) {
        if (handler->handle(aLocation, aResponse))
            break;
    }

} // Wspp::http_location_handle

// ----------------------------------------------------------------------

std::shared_ptr<WsppWebsocketLocationHandler> Wspp::find_create_connected(websocketpp::connection_hdl hdl)
{
    auto connected = mConnected.find(hdl);
    if (connected == mConnected.end()) {
        auto connection = impl->connection(hdl);
        const std::string location = connection->get_resource();
        connected = mConnected.insert({hdl, find_handler_by_location(location).clone()}).first;
        connected->second->set_server_hdl(this, hdl);
    }
    return connected->second;

} // Wspp::find_create_connected

// ----------------------------------------------------------------------

const WsppWebsocketLocationHandler& Wspp::find_handler_by_location(std::string aLocation) const
{
    for (const auto& handler: mWebsocketLocationHandlers) {
        if (handler->use(aLocation)) {
            return *handler;
        }
    }
    throw NoHandlerForLocation{"No handler for location: " + aLocation};

} // Wspp::find_handler_by_location

// ----------------------------------------------------------------------

void Wspp::send(websocketpp::connection_hdl hdl, std::string aMessage, websocketpp::frame::opcode::value op_code)
{
    impl->send(hdl, aMessage, op_code);

} // Wspp::send

// ----------------------------------------------------------------------

bool WsppHttpLocationHandler404::handle(std::string aLocation, WsppHttpResponseData& aResponse)
{
    aResponse.body = "<!doctype html><html><head><title>Error 404 (Resource not found)</title><body><h1>Error 404</h1><p>The requested URL " + aLocation + " was not found on this server.</p></body></head></html>";
    aResponse.status = websocketpp::http::status_code::not_found;
    return true;

} // WsppHttpLocationHandler404::handle

// ----------------------------------------------------------------------

bool WsppHttpLocationHandlerFile::handle(std::string aLocation, WsppHttpResponseData& aResponse)
{
    bool handled = false;
    if (aLocation == mLocation) {
        for (const auto& filename: mFiles) {
            std::ifstream file{filename.c_str()};
            if (file) {
                file.seekg(0, std::ios::end);
                aResponse.body.reserve(static_cast<size_t>(file.tellg()));
                file.seekg(0, std::ios::beg);
                aResponse.body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                if (filename.substr(filename.size() - 3) == ".gz")
                    aResponse.append_header("Content-Encoding", "gzip");
                handled = true;
                break;
            }
        }
    }
    return handled;

} // WsppHttpLocationHandlerFile::handle

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::on_open()
{
    auto connection = mWspp->impl->connection(mHdl);

    using websocketpp::lib::bind;
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;

    connection->set_message_handler(bind(&WsppWebsocketLocationHandler::on_message, this, _1, _2));
    connection->set_close_handler(bind(&WsppWebsocketLocationHandler::on_close, this, _1));

      // $$ add entry to queue

    opening();

} // WsppWebsocketLocationHandler::on_open

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::on_message(websocketpp::connection_hdl /*hdl*/, websocketpp::config::asio::message_type::ptr msg)
{
      // $$ add entry to queue
    std::cerr << "MSG (op-code: " << msg->get_opcode() << "): \"" << msg->get_payload() << '"' << std::endl;
    message(msg->get_payload());

} // WsppWebsocketLocationHandler::on_message

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::on_close(websocketpp::connection_hdl /*hdl*/)
{
      // remove hdl from mWspp->mConnected
      // notify running threads if they process this hdl
      // add entry to queue
    after_close();

} // WsppWebsocketLocationHandler::on_close

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::send(std::string aMessage, websocketpp::frame::opcode::value op_code)
{
      // std::unique_lock<std::mutex> lock{mHdlAccess}; // main thread may change mHdl?
    mWspp->send(mHdl, aMessage, op_code);

} // WsppWebsocketLocationHandler::send

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: