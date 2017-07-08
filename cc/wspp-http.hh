#pragma once

#include <memory>
#include <thread>

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
// for websocketpp::frame::opcode, websocketpp::config::asio::message_type
#include "websocketpp/config/asio.hpp"
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

namespace websocketpp {
    namespace lib {
        using std::weak_ptr;
    }
    using connection_hdl = lib::weak_ptr<void>;
}

// ----------------------------------------------------------------------

class WsppHttpResponseData;
class WsppHttpLocationHandler;
class WsppWebsocketLocationHandler;
namespace _wspp_internal { class WsppImplementation; }      // defined in wspp-http.cc

class Wspp
{
 public:
    Wspp(std::string aHost, std::string aPort, size_t aNumberOfThreads);
    ~Wspp();

    inline void add_location_handler(std::shared_ptr<WsppHttpLocationHandler> aHandler) { mHttpLocationHandlers.push_back(aHandler); }
    inline void add_location_handler(std::shared_ptr<WsppWebsocketLocationHandler> aHandler) { mWebsocketLocationHandlers.push_back(aHandler); }
    void setup_logging(std::string access_log_filename = std::string{}, std::string error_log_filename = std::string{});
    inline _wspp_internal::WsppImplementation& implementation() { return *impl; }

    void run();

    class NoHandlerForLocation : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class NoHandlerForConnection : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class HandlerForConnectionAlreadyExists : public std::runtime_error { public: using std::runtime_error::runtime_error; };

 private:
    std::unique_ptr<_wspp_internal::WsppImplementation> impl;
    std::string certificate_chain_file;
    std::string private_key_file;
    std::string tmp_dh_file;
    std::vector<std::shared_ptr<WsppHttpLocationHandler>> mHttpLocationHandlers;
    std::vector<std::shared_ptr<WsppWebsocketLocationHandler>> mWebsocketLocationHandlers;
    std::map<websocketpp::connection_hdl, std::shared_ptr<WsppWebsocketLocationHandler>, std::owner_less<websocketpp::connection_hdl>> mConnected;
    std::mutex mConnectedAccess;

    void http_location_handle(std::string aLocation, WsppHttpResponseData& aResponse);
    void create_connected(websocketpp::connection_hdl hdl);
    void remove_connected(websocketpp::connection_hdl hdl);
    WsppWebsocketLocationHandler& find_connected(websocketpp::connection_hdl hdl);
    const WsppWebsocketLocationHandler& find_handler_by_location(std::string aLocation) const;

    friend class _wspp_internal::WsppImplementation;
    friend class WsppWebsocketLocationHandler;

}; // class Wspp

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
    inline WsppWebsocketLocationHandler() : mWspp{nullptr}, mOpened{false} {}
    inline WsppWebsocketLocationHandler(const WsppWebsocketLocationHandler& aSrc) : mWspp{aSrc.mWspp} {}
    virtual inline ~WsppWebsocketLocationHandler() {}

 protected:
    virtual std::shared_ptr<WsppWebsocketLocationHandler> clone() const = 0;

      // returns true if this handler is going to handle ws for the passed location, runs in the main thread
    virtual bool use(std::string aLocation) const = 0;

      // run in a child thread
    virtual void opening(std::string) = 0;
    virtual void message(std::string aMessage) = 0;
      // websocket was already closed, no way to send message back
    virtual void after_close(std::string) {}

    void send(std::string aMessage, websocketpp::frame::opcode::value op_code = websocketpp::frame::opcode::text);

 private:
    Wspp* mWspp;
    websocketpp::connection_hdl mHdl;
    bool mOpened;
    std::mutex mAccess;

    inline void set_server_hdl(Wspp* aWspp, websocketpp::connection_hdl aHdl) { std::unique_lock<std::mutex> lock{mAccess}; mWspp = aWspp; mHdl = aHdl; mOpened = true; }
    inline void closed() { std::unique_lock<std::mutex> lock{mAccess}; mOpened = false; } // immeditely called on receiving close event in the main thread
    void open_queue_element_handler(std::string);
    void on_message(websocketpp::connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg);
    void on_close(websocketpp::connection_hdl hdl);
    void call_after_close(std::string aMessage);

    friend class Wspp;
    friend class _wspp_internal::WsppImplementation;

}; // class WsppHttpLocationHandler

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
