#pragma once

#include <functional>
#include <iostream>
#include <ctime>

#include "uws.hh"

// ----------------------------------------------------------------------

class HttpRequestDispatcherBase
{
 public:
    inline HttpRequestDispatcherBase(uWS::Group<uWS::SERVER>* aGroup) //: mGroup(aGroup)
        {
            using namespace std::placeholders;
            aGroup->onHttpRequest(std::bind(&HttpRequestDispatcherBase::dispatcher, this, _1, _2, _3, _4, _5));
            // void onHttpConnection(std::function<void(HttpSocket<isServer> *)> handler);
            // void onHttpData(std::function<void(HttpResponse *, char *data, size_t length, size_t remainingBytes)> handler);
            // void onHttpDisconnection(std::function<void(HttpSocket<isServer> *)> handler);
            // void onCancelledHttpRequest(std::function<void(HttpResponse *)> handler);
            // void onHttpUpgrade(std::function<void(HttpSocket<isServer> *, HttpRequest)> handler);
        }
    virtual inline ~HttpRequestDispatcherBase() {}

 protected:
    virtual void dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* post_data, size_t post_data_length, size_t post_remaining_bytes) = 0;

}; // class HttpRequestDispatcherBase

// ----------------------------------------------------------------------

class HttpRequestRedirectToHttps : public HttpRequestDispatcherBase
{
 public:
      // target port 0 means do not specify port in the new address
    inline HttpRequestRedirectToHttps(uWS::Group<uWS::SERVER>* aGroup, int aTargetPort = 0) : HttpRequestDispatcherBase{aGroup}, mTargetPort{aTargetPort} {}

 protected:
    virtual void dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* post_data, size_t post_data_length, size_t post_remaining_bytes);

 private:
    int mTargetPort;

}; // class HttpRequestRedirectToHttps

// ----------------------------------------------------------------------

class HttpRequestDispatcher : public HttpRequestDispatcherBase
{
 public:
    using HttpRequestDispatcherBase::HttpRequestDispatcherBase;

 protected:
    virtual void dispatcher(uWS::HttpResponse* response, uWS::HttpRequest request, char* post_data, size_t post_data_length, size_t post_remaining_bytes);

 private:
    void get_f(uWS::HttpResponse* response, uWS::HttpRequest request, std::string path);
    std::string content_type(std::string path) const;

}; // class HttpRequestDispatcher

// ----------------------------------------------------------------------

std::ostream& operator << (std::ostream& out, const uWS::HttpRequest& request);

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
