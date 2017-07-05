#include <iostream>
#include <iomanip>
#include <chrono>

#include "http-request-dispatcher.hh"

// ----------------------------------------------------------------------

void HttpRequestRedirectToHttps::dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* /*post_data*/, size_t /*post_data_length*/, size_t /*post_remaining_bytes*/)
{
    // report_request(request);
    std::string host = request.getHeader("host").toString();
    host.erase(host.find(':')); // remove original port
    if (mTargetPort)
        host += ":" + std::to_string(mTargetPort);
    const std::string header = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://" + host + request.getUrl().toString() + "\r\n\r\n";
    response->write(header.c_str(), header.size());
    response->end();

} // HttpRequestRedirectToHttps::dispatcher

// ----------------------------------------------------------------------

void HttpRequestDispatcher::dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* post_data, size_t post_data_length, size_t post_remaining_bytes)
{
    log(request);
    bool handled = false;
    for (auto app: mApps) {
        if ((handled = app->handle(*response, request, post_data, post_data_length, post_remaining_bytes)))
            break;
    }
    if (!handled)
        reply_404(*response);

    //   // report_request(request);
    // const std::string path = request.getUrl().toString();
    // if (path.substr(0, 3) == "/f/") {
    //     get_f(response, request, path.substr(1));
    // }
    // else if (path == "/favicon.ico") {
    //     get_f(response, request, path.substr(1));
    // }
    // else {
    //     std::time_t now = std::time(nullptr);
    //     std::string reply = std::string{"<html><head><script src=\"/f/myscript.js\"></script></head><body><h1>UWS-TEST</h1><p>"} + std::asctime(std::localtime(&now)) + "</p></body></html>";
    //     response->end(reply.c_str(), reply.size());
    // }

} // HttpRequestDispatcher::dispatcher

// ----------------------------------------------------------------------

void HttpRequestDispatcher::reply_404(uWS::HttpResponse& response)
{
    std::string header = "HTTP/1.1 404 Not Found\r\n\r\n";
    response.write(header.c_str(), header.size());
    std::string reply = "<!DOCTYPE HTML><html lang=\"en\"><head><title>The page was not found!</title></head><body><h1>The page was not found!</h1></body></html>";
    response.end(reply.c_str(), reply.size());

} // HttpRequestDispatcher::reply_404

// ----------------------------------------------------------------------

void HttpRequestDispatcher::log(uWS::HttpRequest& request)
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << ' ' << request.getHeader("user-agent").toString() << std::endl;
      //std::cout << request << std::endl;

} // HttpRequestDispatcher::log

// ----------------------------------------------------------------------

// void HttpRequestDispatcher::get_f(uWS::HttpResponse* response, uWS::HttpRequest /*request*/, std::string path)
// {
//     std::string reply;
//     try {
//         struct stat st;
//         std::string filename = path;
//         if (stat(filename.c_str(), &st)) {
//             filename += ".gz";
//             if (stat(filename.c_str(), &st))
//                 throw std::runtime_error(std::strerror(errno));
//         }
//         int fd = open(filename.c_str(), O_RDONLY);
//         if (fd < 0)
//             throw std::runtime_error(std::strerror(errno));
//         reply.resize(static_cast<std::string::size_type>(st.st_size));
//         if (read(fd, const_cast<char*>(reply.data()), reply.size()) != static_cast<ssize_t>(reply.size()))
//             throw std::runtime_error(std::strerror(errno));
//         std::string header;
//         if (reply.size() > 2 && reply[0] == '\x1F' && reply[1] == '\x8B') { // gzip http://www.zlib.org/rfc-gzip.html
//             header = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(reply.size()) + "\r\nContent-Encoding: gzip\r\nContent-Type: " + content_type(path) + "\r\n\r\n";
//         }
//         else {
//             header = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(reply.size()) + "\r\nContent-Type: " + content_type(path) + "\r\n\r\n";
//         }
//         response->write(header.c_str(), header.size());
//     }
//     catch (std::exception& err) {
//         reply = "console.error('ERROR: server cannot read " + path + ": " + err.what() + "');";
//     }
//     response->end(reply.c_str(), reply.size());
// }

// // ----------------------------------------------------------------------

// std::string HttpRequestDispatcher::content_type(std::string path) const
// {
//     if (path.substr(path.size() - 3) == ".js")
//         return "text/javascript";
//     else if (path.substr(path.size() - 4) == ".ico")
//         return "image/x-icon";
//     else
//         return "text/html";

// } // HttpRequestDispatcher::content_type

// ----------------------------------------------------------------------

std::ostream& operator << (std::ostream& out, const uWS::HttpRequest& request)
{
    // const std::time_t now = std::time(nullptr);
    // out << "Request " << std::asctime(std::localtime(&now));
    // out << "URL: " << request.getUrl().toString() << " HOST:" << request.getHeader("host").toString() << std::endl;
    if (request.headers) {
        for (auto* h = request.headers; *h; ++h) {
            out  << std::endl << "  " << std::string(h->key, h->keyLength) << ": " << std::string(h->value, h->valueLength);
        }
    }
    return out;

} // report_headers

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
