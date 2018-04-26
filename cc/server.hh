#pragma once

#include <memory>
#include <thread>
#include <shared_mutex>

//#include "acmacs-webserver/websocketpp-asio.hh"
#include "websocketpp-asio.hh"

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

class Wspp;
class WsppThread;
using WsppThreadMaker = std::function<WsppThread*(Wspp&)>;

// ----------------------------------------------------------------------

class Wspp
{
 public:
    Wspp(const ServerSettings& aSettings, WsppThreadMaker aThreadMaker);
    Wspp(std::string aHost, std::string aPort, size_t aNumberOfThreads, std::string aCerficateChainFile, std::string aPrivateKeyFile, std::string aTmpDhFile, WsppThreadMaker aThreadMaker);
    ~Wspp();

    void add_location_handler(std::shared_ptr<WsppHttpLocationHandler> aHandler) { mHttpLocationHandlers.push_back(aHandler); }
    void add_location_handler(std::shared_ptr<WsppWebsocketLocationHandler> aHandler) { mWebsocketLocationHandlers.push_back(aHandler); }
    void setup_logging(std::string access_log_filename = std::string{}, std::string error_log_filename = std::string{});
    _wspp_internal::WsppImplementation& implementation() { return *impl; }
    void stop_listening();

    void run();

    class NoHandlerForLocation : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class NoHandlerForConnection : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class HandlerForConnectionAlreadyExists : public std::runtime_error { public: using std::runtime_error::runtime_error; };

 private:
    std::unique_ptr<_wspp_internal::WsppImplementation> impl;
    std::string mHost, mPort;
    std::string certificate_chain_file;
    std::string private_key_file;
    std::string tmp_dh_file;
    std::vector<std::shared_ptr<WsppHttpLocationHandler>> mHttpLocationHandlers;
    std::vector<std::shared_ptr<WsppWebsocketLocationHandler>> mWebsocketLocationHandlers;
    std::map<websocketpp::connection_hdl, std::shared_ptr<WsppWebsocketLocationHandler>, std::owner_less<websocketpp::connection_hdl>> mConnected;
    std::mutex mConnectedAccess;

    void read_settings(const ServerSettings& aSettings, WsppThreadMaker aThreadMaker);
    void http_location_handle(const HttpResource& aResource, WsppHttpResponseData& aResponse);
    std::shared_ptr<WsppWebsocketLocationHandler> create_connected(websocketpp::connection_hdl hdl);
    void remove_connected(websocketpp::connection_hdl hdl);
    std::shared_ptr<WsppWebsocketLocationHandler> find_connected(websocketpp::connection_hdl hdl);
    const WsppWebsocketLocationHandler& find_handler_by_location(std::string aLocation) const;

    friend class _wspp_internal::WsppImplementation;
    friend class WsppWebsocketLocationHandler;

}; // class Wspp

// ----------------------------------------------------------------------

class WsppThread
{
 public:
    virtual ~WsppThread();
    static inline WsppThread* make(Wspp& aWspp) { return new WsppThread{aWspp}; }

 protected:
    WsppThread(Wspp& aWspp) : mWspp{aWspp}, mThread{std::bind(&WsppThread::run, this)} {}

    virtual void initialize();

 private:
    Wspp& mWspp;
    std::thread mThread;

    [[noreturn]] void run();

}; // class WsppThread

// ----------------------------------------------------------------------

class HttpResource
{
 public:
    using Argv = std::map<std::string, std::vector<std::string>>;

    HttpResource(std::string aResource);

    std::string location() const { return mLocation; }
    const Argv& argv() const { return mArgv; }

 private:
    std::string mLocation;
    Argv mArgv;

}; // class HttpResource

// ----------------------------------------------------------------------

class WsppHttpResponseData
{
 public:
    WsppHttpResponseData(websocketpp::http::status_code::value aStatus = websocketpp::http::status_code::ok) : status(aStatus) {}
    void append_header(std::string key, std::string value) { headers.emplace_back(key, value); }

    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;
    websocketpp::http::status_code::value status;

}; // class WsppHttpLocationHandler

class WsppHttpLocationHandler
{
 public:
    WsppHttpLocationHandler() = default;
    virtual ~WsppHttpLocationHandler() = default;

    virtual bool handle(const HttpResource& aResource, WsppHttpResponseData& aResponse) = 0;

}; // class WsppHttpLocationHandler

class WsppHttpLocationHandler404 : public WsppHttpLocationHandler
{
 public:
    bool handle(const HttpResource& aResource, WsppHttpResponseData& aResponse) override;

}; // class WsppHttpLocationHandler404

class WsppHttpLocationHandlerFile : public WsppHttpLocationHandler
{
 public:
    WsppHttpLocationHandlerFile(std::string aLocation, const std::vector<std::string>& aFiles)
        : mLocation{aLocation}, mFiles{aFiles} {}
    template <typename Iter> WsppHttpLocationHandlerFile(std::string aLocation, Iter first_file, Iter last_file)
        : mLocation{aLocation}, mFiles(first_file, last_file) {}

    bool handle(const HttpResource& aResource, WsppHttpResponseData& aResponse) override;

 private:
    std::string mLocation;
    std::vector<std::string> mFiles;

}; // class WsppHttpLocationHandler404

// ----------------------------------------------------------------------

class WsppWebsocketLocationHandler
{
 public:
    WsppWebsocketLocationHandler() : mWspp{nullptr} {}
    WsppWebsocketLocationHandler(const WsppWebsocketLocationHandler& aSrc) : mWspp{aSrc.mWspp} {}
    virtual ~WsppWebsocketLocationHandler();

    void send(std::string aMessage, websocketpp::frame::opcode::value op_code = websocketpp::frame::opcode::text);

 protected:
    virtual std::shared_ptr<WsppWebsocketLocationHandler> clone() const = 0;

      // returns true if this handler is going to handle ws for the passed location, runs in the main thread
    virtual bool use(std::string aLocation) const = 0;

      // run in a child thread
    virtual void opening(std::string, WsppThread& aThread) = 0;
    virtual void message(std::string aMessage, WsppThread& aThread) = 0;
      // websocket was already closed, no way to send message back
    virtual void after_close(std::string, WsppThread& /*aThread*/) {}

 private:
    Wspp* mWspp;
    websocketpp::connection_hdl mHdl;
    std::mutex mAccess;

    class ConnectionClosed : public std::exception { public: using std::exception::exception; };

    void set_server_hdl(Wspp* aWspp, websocketpp::connection_hdl aHdl) { std::unique_lock<decltype(mAccess)> lock{mAccess}; mWspp = aWspp; mHdl = aHdl; }
    void closed(); // may call (indirectly) destructor for this, caller needs to lock mAccess
    void open_queue_element_handler(std::string, WsppThread& aThread);
    void on_message(websocketpp::connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg);
    void on_close(websocketpp::connection_hdl hdl);
    void call_after_close(std::string aMessage, WsppThread& aThread);
    _wspp_internal::WsppImplementation& wspp_implementation() // caller needs to lock mAccess
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
