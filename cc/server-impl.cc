#include "server-impl.hh"
#include "server.hh"

using namespace _wspp_internal;

// ----------------------------------------------------------------------

WsppImplementation::WsppImplementation(Wspp& aParent, size_t aNumberOfThreads, WsppThreadMaker aThreadMaker)
    : mParent{aParent}, mThreads{aParent, aNumberOfThreads, aThreadMaker}
{
    mServer.init_asio();
    mServer.set_reuse_addr(true);

    using websocketpp::lib::bind;
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;

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
          // print_cerr("using TLS mode: ", (mode == MOZILLA_MODERN ? "Mozilla Modern" : "Mozilla Intermediate"));
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
            print_cerr("Error setting cipher list");
        }

    }
    catch (std::exception& e) {
        print_cerr("Exception in initializing TLS: ", e.what());
        throw;
    }
    return ctx;

} // WsppImplementation::on_tls_init

// ----------------------------------------------------------------------

void WsppImplementation::on_http(websocketpp::connection_hdl hdl)
{
    auto connection = mServer.get_con_from_hdl(hdl);
    HttpResource resource{connection->get_resource()};

    WsppHttpResponseData response;
    mParent.http_location_handle(resource, response);
    for (const auto& header: response.headers)
        connection->append_header(header.first, header.second);
    connection->set_body(response.body);
    connection->set_status(response.status);

        //   // POST
        // std::string res = con->get_request_body();
        // print_cerr("got HTTP request with ", res.size(), " bytes of body data.");
      // print_cerr("http secure: ", con->get_secure());
      // print_cerr("http host: \"", con->get_host(), '"');
      // print_cerr("http port: ", con->get_port());
      // print_cerr("http resource: \"", connection->get_resource(), '"'); // location in URL
      // print_cerr("http header: \"", con->get_request_header("???"), '"');
      // print_cerr("http origin: \"", con->get_origin(), '"'); // SEG FAULT

} // WsppImplementation::on_http

// ----------------------------------------------------------------------

void WsppImplementation::on_open(websocketpp::connection_hdl hdl)
{
    try {
        mQueue.push(mParent.create_connected(hdl), &WsppWebsocketLocationHandler::open_queue_element_handler);
    }
    catch (std::exception& err) {
        print_cerr("error opening connection: ", err.what());
        close(hdl, err.what());
    }

} // WsppImplementation::on_open

// ----------------------------------------------------------------------

  // runs in the thread
void WsppImplementation::pop_call(WsppThread& aThread)
{
    auto data = mQueue.pop(); // blocks on waiting for a data in the queue
    if (data.connected->mWspp)  // check if not already closed (in antoher thread), otherwise pure virtual function call occurs
        (data.connected.get()->*data.handler)(data.message, aThread);

} // WsppImplementation::pop_call

// ----------------------------------------------------------------------

void Threads::start()
{
    print_cerr("Starting ", size(), " worker threads");
    std::transform(this->begin(), this->end(), this->begin(), [this](const auto&) {
            // using namespace std::chrono_literals;
            // std::this_thread::sleep_for(0.2s);
            return std::shared_ptr<WsppThread>(this->mThreadMaker(this->mWspp));
        });

} // Threads::start

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
