#include <string>
#include <iostream>
#include "uws.hh"

int main()
{
    uWS::Hub h;

    h.onHttpRequest([](uWS::HttpResponse* respose, uWS::HttpRequest request, char *data, size_t length, size_t remainingBytes) {
            std::cout << "URL: " << request.getUrl().toString() << " HOST:" << request.getHeader("host").toString() << std::endl;
            if (data)
                std::cout << "REQ data: " << data << std::endl;
            std::string reply = "<html><body><h1>UWS-TEST</h1></body></html>";
            respose->end(reply.c_str(), reply.size());
        });

    h.onMessage([](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
            ws->send(message, length, opCode);
        });

    h.listen(3000);
    h.run();
    return 0;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
