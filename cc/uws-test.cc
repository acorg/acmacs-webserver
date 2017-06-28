#include <string>
#include <iostream>
#include "uws.hh"

void report_request(uWS::HttpRequest& request);

int main()
{
    try {
        uWS::Hub h;
        uWS::Group<uWS::SERVER>* sslGroup = h.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        uWS::Group<uWS::SERVER>* group = h.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);

          // certChainFileName, keyFileName, keyFilePassword = ""
        uS::TLS::Context tls_context = uS::TLS::createContext("/Users/eu/Desktop/AcmacsWeb.app/Contents/etc/self-signed.crt", "/Users/eu/Desktop/AcmacsWeb.app/Contents/etc/self-signed.key", "");

        sslGroup->onHttpRequest([](uWS::HttpResponse* response, uWS::HttpRequest request, char* /*post_data*/, size_t /*post_data_length*/, size_t /*post_remaining_bytes*/) {
                std::cout << "Request HTTPS" << std::endl;
                report_request(request);
                std::string reply = "<html><body><h1>UWS-TEST</h1><p>XX</p></body></html>";
                response->end(reply.c_str(), reply.size());
            });

        group->onHttpRequest([](uWS::HttpResponse* response, uWS::HttpRequest request, char* /*post_data*/, size_t /*post_data_length*/, size_t /*post_remaining_bytes*/) {
                  // redirect to https://<host>:3000
                std::string host = request.getHeader("host").toString();
                  // std::cout << "Request HTTP " << host << std::endl;
                host.back() = '0';
                std::string reply = "<html><head><meta http-equiv=\"Refresh\" content=\"0;URL=https://" + host + request.getUrl().toString() + "\" /></head></html>";
                response->end(reply.c_str(), reply.size());
            });

        // h.onMessage([](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
        //         ws->send(message, length, opCode);
        //     });

        if (!h.listen(3001, nullptr, 0, group))
            throw std::runtime_error("Listening http failed");
        if (!h.listen(3000, tls_context, 0, sslGroup))
            throw std::runtime_error("Listening https failed");

        std::cout << "Listening" << std::endl;
        h.run();
        return 0;
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << std::endl;
        return 1;
    }
}

// ----------------------------------------------------------------------

void report_request(uWS::HttpRequest& request)
{
    // std::cout << "URL: " << request.getUrl().toString() << " HOST:" << request.getHeader("host").toString() << std::endl;
    if (request.headers) {
        for (auto* h = request.headers; *h; ++h) {
            std::cout << std::string(h->key, h->keyLength) << ": " << std::string(h->value, h->valueLength) << std::endl;
        }
    }

} // report_headers

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
