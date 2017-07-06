#pragma once

#include <memory>

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
#include "websocketpp/http/constants.hpp"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
#include "websocketpp/config/asio.hpp"
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

namespace websocketpp {
    // namespace config {
    //     struct asio_tls;
    // }
    namespace lib {
        using std::weak_ptr;
    }
    template <typename config> class server;
    using connection_hdl = lib::weak_ptr<void>;
}

// ----------------------------------------------------------------------

class WsppHttpResponseData
{
 public:
    inline WsppHttpResponseData(websocketpp::http::status_code::value aStatus = websocketpp::http::status_code::ok) : status(aStatus) {}
    inline void append_header(std::string key, std::string value) { headers.emplace_back(key, value); }

    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;
    websocketpp::http::status_code::value status;

}; // class WsppHttpLocationHandler

class WsppHttpLocationHandler
{
 public:
    inline WsppHttpLocationHandler() = default;
    virtual inline ~WsppHttpLocationHandler() {}

    virtual bool handle(std::string aLocation, WsppHttpResponseData& aResponse) = 0;

}; // class WsppHttpLocationHandler

class WsppHttpLocationHandler404 : public WsppHttpLocationHandler
{
 public:
    virtual bool handle(std::string aLocation, WsppHttpResponseData& aResponse);

}; // class WsppHttpLocationHandler404

class WsppHttpLocationHandlerFile : public WsppHttpLocationHandler
{
 public:
    inline WsppHttpLocationHandlerFile(std::string aLocation, const std::vector<std::string>& aFiles)
        : mLocation{aLocation}, mFiles{aFiles} {}

    virtual bool handle(std::string aLocation, WsppHttpResponseData& aResponse);

 private:
    std::string mLocation;
    std::vector<std::string> mFiles;

}; // class WsppHttpLocationHandler404

// ----------------------------------------------------------------------

class WsppWebsocketLocationHandler
{
 public:
    inline WsppWebsocketLocationHandler() : mServer {nullptr} {}
    virtual inline ~WsppWebsocketLocationHandler() {}

 protected:
      // returns true if this handler is going to handle ws for the passed location
    virtual bool use(std::string aLocation) = 0;

    virtual void on_open(websocketpp::connection_hdl hdl) = 0;
    virtual void on_close(websocketpp::connection_hdl /*hdl*/) {}
    virtual void on_message(websocketpp::connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg) = 0;

    void send(websocketpp::connection_hdl hdl, std::string aMessage, websocketpp::frame::opcode::value op_code = websocketpp::frame::opcode::text);

 private:
    using server = websocketpp::server<websocketpp::config::asio_tls>;
    server* mServer;

    friend class WsppHttp;

}; // class WsppHttpLocationHandler

// ----------------------------------------------------------------------

class WsppHttp
{
 public:
    using server = websocketpp::server<websocketpp::config::asio_tls>;

    WsppHttp(std::string aHost, std::string aPort);
    ~WsppHttp();

    inline void add_location_handler(std::shared_ptr<WsppHttpLocationHandler> aHandler) { mHttpLocationHandlers.push_back(aHandler); }
    inline void add_location_handler(std::shared_ptr<WsppWebsocketLocationHandler> aHandler) { mWebsocketLocationHandlers.push_back(aHandler); }
    void setup_logging(std::string access_log_filename = std::string{}, std::string error_log_filename = std::string{});

    void run();

      // must be public
    std::string certificate_chain_file;
    std::string private_key_file;
    std::string tmp_dh_file;

 private:
    server* mServer;
    std::vector<std::shared_ptr<WsppHttpLocationHandler>> mHttpLocationHandlers;
    std::vector<std::shared_ptr<WsppWebsocketLocationHandler>> mWebsocketLocationHandlers;

    void on_http(websocketpp::connection_hdl hdl);
    void on_open(websocketpp::connection_hdl hdl);
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
