#pragma once

#include <memory>
#include <thread>
#include <shared_mutex>

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
class ServerSettings;
class HttpResource;

class Wspp
{
 public:
    Wspp(const ServerSettings& aSettings);
    Wspp(std::string aHost, std::string aPort, size_t aNumberOfThreads, std::string aCerficateChainFile, std::string aPrivateKeyFile, std::string aTmpDhFile);
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

    void read_settings(const ServerSettings& aSettings);
    void http_location_handle(const HttpResource& aResource, WsppHttpResponseData& aResponse);
    std::shared_ptr<WsppWebsocketLocationHandler> create_connected(websocketpp::connection_hdl hdl);
    void remove_connected(websocketpp::connection_hdl hdl);
    std::shared_ptr<WsppWebsocketLocationHandler> find_connected(websocketpp::connection_hdl hdl);
    const WsppWebsocketLocationHandler& find_handler_by_location(std::string aLocation) const;

    friend class _wspp_internal::WsppImplementation;
    friend class WsppWebsocketLocationHandler;

}; // class Wspp

// ----------------------------------------------------------------------

class HttpResource
{
 public:
    using Argv = std::map<std::string, std::vector<std::string>>;

    HttpResource(std::string aResource);

    inline std::string location() const { return mLocation; }
    inline const Argv& argv() const { return mArgv; }

 private:
    std::string mLocation;
    Argv mArgv;

}; // class HttpResource

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

    virtual bool handle(const HttpResource& aResource, WsppHttpResponseData& aResponse) = 0;

}; // class WsppHttpLocationHandler

class WsppHttpLocationHandler404 : public WsppHttpLocationHandler
{
 public:
    virtual bool handle(const HttpResource& aResource, WsppHttpResponseData& aResponse);

}; // class WsppHttpLocationHandler404

class WsppHttpLocationHandlerFile : public WsppHttpLocationHandler
{
 public:
    inline WsppHttpLocationHandlerFile(std::string aLocation, const std::vector<std::string>& aFiles)
        : mLocation{aLocation}, mFiles{aFiles} {}

    virtual bool handle(const HttpResource& aResource, WsppHttpResponseData& aResponse);

 private:
    std::string mLocation;
    std::vector<std::string> mFiles;

}; // class WsppHttpLocationHandler404

// ----------------------------------------------------------------------

class WsppWebsocketLocationHandler
{
 public:
    inline WsppWebsocketLocationHandler() : mWspp{nullptr} {}
    inline WsppWebsocketLocationHandler(const WsppWebsocketLocationHandler& aSrc) : mWspp{aSrc.mWspp} {}
    virtual inline ~WsppWebsocketLocationHandler() {}

    void send(std::string aMessage, websocketpp::frame::opcode::value op_code = websocketpp::frame::opcode::text);

 protected:
    virtual std::shared_ptr<WsppWebsocketLocationHandler> clone() const = 0;

      // returns true if this handler is going to handle ws for the passed location, runs in the main thread
    virtual bool use(std::string aLocation) const = 0;

      // run in a child thread
    virtual void opening(std::string) = 0;
    virtual void message(std::string aMessage) = 0;
      // websocket was already closed, no way to send message back
    virtual void after_close(std::string) {}

 private:
    Wspp* mWspp;
    websocketpp::connection_hdl mHdl;
    std::mutex mAccess;

    class ConnectionClosed : public std::exception { public: using std::exception::exception; };

    inline void set_server_hdl(Wspp* aWspp, websocketpp::connection_hdl aHdl) { std::unique_lock<decltype(mAccess)> lock{mAccess}; mWspp = aWspp; mHdl = aHdl; }
    void closed(); // may call (indirectly) destructor for this, caller needs to lock mAccess
    void open_queue_element_handler(std::string);
    void on_message(websocketpp::connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg);
    void on_close(websocketpp::connection_hdl hdl);
    void call_after_close(std::string aMessage);
    inline _wspp_internal::WsppImplementation& wspp_implementation() // caller needs to lock mAccess
        {
            if (!mWspp || mHdl.expired())
                throw ConnectionClosed{};
            return mWspp->implementation();
        }

    friend class Wspp;
    friend class _wspp_internal::WsppImplementation;

}; // class WsppHttpLocationHandler

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
