#include <iostream>
#include <fstream>
#include <chrono>
#include <typeinfo>

#include "acmacs-base/stream.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/string.hh"

#include "server.hh"
#include "server-settings.hh"
#include "server-impl.hh"
#include "print.hh"

// ----------------------------------------------------------------------

using namespace _wspp_internal;

// ----------------------------------------------------------------------

Wspp::Wspp(const ServerSettings& aSettings, WsppThreadMaker aThreadMaker)
{
    read_settings(aSettings, aThreadMaker);

} // Wspp::Wspp

// ----------------------------------------------------------------------

Wspp::Wspp(std::string aHost, std::string aPort, size_t aNumberOfThreads, std::string aCerficateChainFile, std::string aPrivateKeyFile, std::string aTmpDhFile, WsppThreadMaker aThreadMaker)
    : impl{new WsppImplementation{*this, aNumberOfThreads, aThreadMaker}},
      mHost{aHost}, mPort{aPort},
      certificate_chain_file{aCerficateChainFile}, private_key_file{aPrivateKeyFile}, tmp_dh_file{aTmpDhFile}
{
} // Wspp::Wspp

// ----------------------------------------------------------------------

Wspp::~Wspp()
{
      // print_cerr("~Wspp");
    if (log_send_receive_ != &std::cout && log_send_receive_ != &std::cerr)
        delete log_send_receive_;

} // Wspp::~Wspp

// ----------------------------------------------------------------------

void Wspp::read_settings(const ServerSettings& settings, WsppThreadMaker aThreadMaker)
{
    impl = std::make_unique<WsppImplementation>(*this, settings.number_of_threads(), aThreadMaker);
    mHost = settings.host();
    mPort = std::to_string(settings.port());
    certificate_chain_file = settings.certificate_chain_file();
    private_key_file = settings.private_key_file();
    tmp_dh_file = settings.tmp_dh_file();
    setup_logging(settings.log_access(), settings.log_error(), settings.log_send_receive());

    rjson::for_each(settings.locations(), [this](const rjson::value& location) {
        try {
            if (const auto& loc = rjson::get_or(location, "location", ""); !loc.empty()) {
                if (const auto& files = location["files"]; !files.empty()) {
                    add_location_handler(std::make_shared<WsppHttpLocationHandlerFile>(std::string(loc), rjson::as_vector<std::string>(files)));
                }
                else {
                    if (const auto& dirs = location["dirs"]; !dirs.empty() || loc.back() != '/' || dirs[0].to<std::string_view>().back() != '/') {
                        rjson::for_each(dirs, [this, loc = static_cast<std::string>(loc)](const rjson::value& dir) {
                            try {
                                // std::cerr << "DEBUG: scanning " << dir << AD_DEBUG_FILE_LINE_FUNC << '\n';
                                for (auto& entry : fs::directory_iterator(dir.to<std::string_view>())) {
                                    auto& path = entry.path();
                                    if (exists(path)) {
                                        auto filename = path.filename();
                                        if (filename.extension() == ".gz")
                                            filename = filename.stem();
                                        // std::cerr << "DEBUG: add location handler for " << loc + filename.string() << " path: " << path.string() << AD_DEBUG_FILE_LINE_FUNC << '\n';
                                        add_location_handler(std::make_shared<WsppHttpLocationHandlerFile>(loc + filename.string(), std::vector<std::string>{path.string()}));
                                    }
                                    else {
                                        print_cerr("No file: ", path.string());
                                    }
                                }
                            }
                            catch (fs::filesystem_error& err) {
                                throw std::runtime_error(string::concat("directory ", dir.to<std::string_view>(), " access failed: ", err.what()));
                            }
                        });
                    }
                    else {
                        throw std::runtime_error("invalid value for \"files\" or \"dirs\"");
                    }
                }
            }
            else {
                throw std::runtime_error("invalid value for \"location\"");
            }
        }
        catch (std::exception& err) {
            print_cerr("Warning: invalid location entry: ", rjson::to_string(location), ": ", err.what());
        }
    });

} // Wspp::read_settings

// ----------------------------------------------------------------------

void Wspp::setup_logging(std::string_view access_log_filename, std::string_view error_log_filename, std::string_view log_send_receive)
{
    using namespace websocketpp::log;
    auto& alog = implementation().server().get_alog();
    if (!access_log_filename.empty()) {
        if (access_log_filename == "-") {
            alog.set_ostream(&std::cout);
        }
        else if (access_log_filename == "=") {
            alog.set_ostream(&std::cerr);
        }
        else {
            fs::create_directories(fs::path(access_log_filename).parent_path());
            auto* output = new std::ofstream{std::string(access_log_filename), std::ios_base::out | std::ios_base::app};
            if (output && *output)
                alog.set_ostream(output);
            else
                delete output;
        }
    }

    // remove all logging, then enable individual channels

    alog.clear_channels(alevel::all);
    alog.set_channels(alevel::none // ~/AD/include/websocketpp/logger/levels.hpp
                      | alevel::connect |
                      alevel::disconnect
                      // | alevel::control
                      // | alevel::frame_header
                      // | alevel::frame_payload
                      // | alevel::message_header // Reserved
                      // | alevel::message_payload // Reserved
                      // | alevel::endpoint // Reserved
                      // | alevel::debug_handshake
                      // | alevel::debug_close
                      // | alevel::devel
                      | alevel::app    // Special channel for application specific logs. Not used by the library.
                      | alevel::http   // Access related to HTTP requests
                      | alevel::fail); // alevel::connect | alevel::disconnect);

    auto& elog = implementation().server().get_elog();
    if (!error_log_filename.empty()) {
        if (error_log_filename == "-") {
            elog.set_ostream(&std::cout);
        }
        else if (error_log_filename == "=") {
            elog.set_ostream(&std::cerr);
        }
        else {
            fs::create_directories(fs::path(error_log_filename).parent_path());
            auto* fs = new std::ofstream{std::string(error_log_filename), std::ios_base::out | std::ios_base::app};
            if (fs && *fs)
                elog.set_ostream(fs);
            else
                delete fs;
        }
    }
    elog.clear_channels(alevel::all);
    elog.set_channels(alevel::none // ~/AD/include/websocketpp/logger/levels.hpp
                      | alevel::connect | alevel::disconnect | alevel::control | alevel::frame_header | alevel::frame_payload | alevel::message_header | alevel::message_payload | alevel::endpoint |
                      alevel::debug_handshake | alevel::debug_close | alevel::devel | alevel::app | alevel::http // Access related to HTTP requests
                      | alevel::fail);                                                                           // alevel::connect | alevel::disconnect);

    if (!log_send_receive.empty()) {
        if (log_send_receive == "-") {
            log_send_receive_ = &std::cout;
        }
        else if (log_send_receive == "=") {
            log_send_receive_ = &std::cerr;
        }
        else {
            fs::create_directories(fs::path(log_send_receive).parent_path());
            log_send_receive_ = new std::ofstream{std::string(log_send_receive), std::ios_base::out | std::ios_base::ate | std::ios_base::app};
            if (!log_send_receive_ || !log_send_receive_) {
                delete log_send_receive_;
                log_send_receive_ = &std::cerr;
                *log_send_receive_ << "WARNING: cannot open log file: " << log_send_receive << '\n';
            }
        }
    }

} // Wspp::setup_logging

// ----------------------------------------------------------------------

void Wspp::run()
{
    add_location_handler(std::make_shared<WsppHttpLocationHandler404>());
        // $$ add default websocket location handler
    auto& imp = implementation();
    imp.listen(mHost, mPort);
    imp.start_threads();
    imp.server().start_accept();
    imp.server().run();

} // Wspp::run

// ----------------------------------------------------------------------

void Wspp::http_location_handle(const HttpResource& aResource, WsppHttpResponseData& aResponse)
{
    for (auto handler: mHttpLocationHandlers) {
        if (handler->handle(aResource, aResponse))
            break;
    }

} // Wspp::http_location_handle

// ----------------------------------------------------------------------

std::shared_ptr<WsppWebsocketLocationHandler> Wspp::find_connected(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> lock{mConnectedAccess};
    auto connected = mConnected.find(hdl);
    if (connected == mConnected.end())
        throw NoHandlerForConnection{"NoHandlerForConnection (internal)"};
    return connected->second;

} // Wspp::find_connected

// ----------------------------------------------------------------------

std::shared_ptr<WsppWebsocketLocationHandler> Wspp::create_connected(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> lock{mConnectedAccess};
    if (mConnected.find(hdl) != mConnected.end())
        throw HandlerForConnectionAlreadyExists{"HandlerForConnectionAlreadyExists (internal)"}; // internal error
    auto connection = implementation().connection(hdl);
    const std::string location = connection->get_resource();
    auto connected = mConnected.insert({hdl, find_handler_by_location(location).clone()}).first->second;
    connected->set_server_hdl(this, hdl);
      // print_cerr("new connected, total: ", std::to_string(mConnected.size()));
    return connected;

} // Wspp::create_connected

// ----------------------------------------------------------------------

void Wspp::remove_connected(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> lock{mConnectedAccess};
    auto connected = mConnected.find(hdl);
    if (connected == mConnected.end())
        throw NoHandlerForConnection{"NoHandlerForConnection (trying to remove it again?)"};
    mConnected.erase(connected);
      // print_cerr("connected removed, total: ", std::to_string(mConnected.size()));

} // Wspp::remove_connected

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

void Wspp::stop_listening()
{
    implementation().stop_listening();

} // Wspp::stop_listening

// ----------------------------------------------------------------------

bool WsppHttpLocationHandler404::handle(const HttpResource& aResource, WsppHttpResponseData& aResponse)
{
    std::cerr << "Error 404: " << aResource.location() << std::endl;
    aResponse.body = "<!doctype html><html><head><title>Error 404 (Resource not found)</title><body><h1>Error 404</h1><p>The requested URL " + aResource.location() + " was not found on this server.</p></body></head></html>";
    aResponse.status = websocketpp::http::status_code::not_found;
    return true;

} // WsppHttpLocationHandler404::handle

// ----------------------------------------------------------------------

bool WsppHttpLocationHandlerFile::handle(const HttpResource& aResource, WsppHttpResponseData& aResponse)
{
    bool handled = false;
    if (aResource.location() == mLocation) {
        for (const auto& filename: mFiles) {
            std::ifstream file{filename.c_str()};
            if (file) {
                file.seekg(0, std::ios::end);
                aResponse.body.reserve(static_cast<size_t>(file.tellg()));
                file.seekg(0, std::ios::beg);
                aResponse.body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                if (filename.substr(filename.size() - 3) == ".js" || filename.substr(filename.size() - 6) == ".js.gz")
                    aResponse.append_header("Content-Type", "application/javascript; charset=utf-8");
                else if (filename.substr(filename.size() - 4) == ".css" || filename.substr(filename.size() - 7) == ".css.gz")
                    aResponse.append_header("Content-Type", "text/css");
                else if (filename.substr(filename.size() - 5) == ".html")
                    aResponse.append_header("Content-Type", "text/html; charset=utf-8");
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

WsppWebsocketLocationHandler::~WsppWebsocketLocationHandler()
{
      // print_cerr("~WsppWebsocketLocationHandler ", this);

      // remove message and close handle to avoid calling them for destructed object
    try {
        std::unique_lock<decltype(mAccess)> lock{mAccess};
        try {
            auto connection = wspp_implementation().connection(mHdl); // may throw ConnectionClosed
            connection->set_message_handler(websocketpp::connection<websocketpp::config::asio_tls>::message_handler{});
            connection->set_close_handler(websocketpp::close_handler{});
        }
        catch (ConnectionClosed&) {
        }
    }
    catch (std::exception& err) {
        print_cerr("Error in ~WsppWebsocketLocationHandler: ", err.what(), " ", this);
    }

} // WsppWebsocketLocationHandler::~WsppWebsocketLocationHandler

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::closed()
{
    auto wspp = mWspp;
    mWspp = nullptr;
    wspp->remove_connected(mHdl); // may call destructor for this!

} // WsppWebsocketLocationHandler::closed

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::open_queue_element_handler(std::string aMessage, WsppThread& aThread)
{
    std::unique_lock<decltype(mAccess)> lock{mAccess};
    try {
        auto connection = wspp_implementation().connection(mHdl); // may throw ConnectionClosed
        lock.unlock();

        connection->set_message_handler(websocketpp::lib::bind(&WsppWebsocketLocationHandler::on_message, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
        connection->set_close_handler(websocketpp::lib::bind(&WsppWebsocketLocationHandler::on_close, this, websocketpp::lib::placeholders::_1));

        opening(aMessage, aThread);
    }
    catch (ConnectionClosed&) {
            // print_cerr("cannot handle opening: connection already closed");
            // thrown by wspp_implementation when lock is still locked
        closed();
    }

} // WsppWebsocketLocationHandler::open_queue_element_handler

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::on_message(websocketpp::connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg)
{
        // std::cerr << "MSG (op-code: " << msg->get_opcode() << "): \"" << msg->get_payload() << '"' << std::endl;
    try {
        std::unique_lock<decltype(mAccess)> lock{mAccess};
        auto connected = mWspp->find_connected(hdl);
        try {
            wspp_implementation().queue().push(connected, &WsppWebsocketLocationHandler::message, msg->get_payload());
        }
        catch (ConnectionClosed&) {
                // print_cerr("cannot handle message: connection already closed (may it ever happen??)");
            closed();
        }
    }
    catch (std::exception& err) {
        print_cerr("Error handling message: ", err.what());
        closed();
    }

} // WsppWebsocketLocationHandler::on_message

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::on_close(websocketpp::connection_hdl hdl)
{
      // print_cerr("connection closed ", this);
    try {
        std::unique_lock<decltype(mAccess)> lock{mAccess}; // may throw std::system_error on recursive locking by the same thread (main thread in this case)
        auto connected = mWspp->find_connected(hdl);
        try {
            wspp_implementation().queue().push(connected, &WsppWebsocketLocationHandler::call_after_close);
        }
        catch (ConnectionClosed&) {
            print_cerr("on_close event for already closed connection (internal)");
        }
    }
    catch (std::exception& err) {
        print_cerr("Error handling closing: ", err.what(), " ", this);
    }
    closed();
      // print_cerr("connection closed done");

} // WsppWebsocketLocationHandler::on_close

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::call_after_close(std::string aMessage, WsppThread& aThread)
{
      // print_cerr("call_after_close");
    after_close(aMessage, aThread);

} // WsppWebsocketLocationHandler::call_after_close

// ----------------------------------------------------------------------

void WsppWebsocketLocationHandler::send(std::string aMessage, websocketpp::frame::opcode::value op_code)
{
    try {
        std::unique_lock<decltype(mAccess)> lock{mAccess};
        auto& impl = wspp_implementation();
        lock.unlock();
        return impl.send(mHdl, aMessage, op_code);
    }
    catch (ConnectionClosed&) {
    }

} // WsppWebsocketLocationHandler::send

// ----------------------------------------------------------------------

HttpResource::HttpResource(std::string aResource)
{
    auto qmark = aResource.find('?');
    if (qmark == std::string::npos) {
        mLocation = aResource;
    }
    else {
        mLocation = aResource.substr(0, qmark);
        for (size_t substart = qmark + 1, subend = substart; subend < aResource.size(); substart = subend + 1) {
            subend = aResource.find('&', substart);
            if (subend != substart /* std::string::npos */) {
                const auto chunk = aResource.substr(substart, subend - substart);
                const auto eqsign = chunk.find('=');
                const auto key = chunk.substr(0, eqsign);
                const auto value = eqsign == std::string::npos ? std::string{} : chunk.substr(eqsign + 1);
                const auto argv_iter = mArgv.find(key);
                if (argv_iter == mArgv.end())
                    mArgv.emplace(key, Argv::mapped_type{value});
                else
                    argv_iter->second.push_back(value);
            }
        }
    }

      // std::cerr << "HttpResource [" << mLocation << "] " << mArgv << std::endl;

} // HttpResource::HttpResource

// ----------------------------------------------------------------------

WsppThread::~WsppThread()
{
} // WsppThread::~WsppThread

// ----------------------------------------------------------------------

void WsppThread::run()
{
    initialize();
    while (true) {
        try {
            mWspp.implementation().pop_call(*this); // pop() blocks waiting for the message from queue
        }
        catch (std::exception& err) {
            print_cerr("handling failed: (", typeid(err).name(), "): ", err.what());
        }
    }

} // WsppThread::run

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
