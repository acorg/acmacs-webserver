#pragma once

#include <string>
#include <vector>
#include <thread>
#include <fstream>

#include "acmacs-base/rjson.hh"

// ----------------------------------------------------------------------

namespace internal
{
    class Location
    {
     public:
        Location() {}

        std::string location;
        std::vector<std::string> files;

        std::vector<std::string>& files_ref() { return files; } // for from_json
    };
}

class ServerSettings
{
 public:
    ServerSettings(std::string aFilename) : doc_{rjson::parse_file(aFilename)} {}
    virtual ~ServerSettings() = default;

    std::string host() const { return doc_.get_or_default("host", ""); }
    size_t port() const { return doc_.get_or_default("port", 0UL); }
    auto number_of_threads() const { return doc_.get_or_default("number_of_threads", std::thread::hardware_concurrency()); }
    std::string certificate_chain_file() const { return doc_.get_or_default("certificate_chain_file", ""); }
    std::string private_key_file() const { return doc_.get_or_default("private_key_file", ""); }
    std::string tmp_dh_file() const { return doc_.get_or_default("tmp_dh_file", ""); }
    std::string log_access() const { return doc_.get_or_default("log_access", ""); }
    std::string log_error() const { return doc_.get_or_default("log_error", ""); }
    std::string log_send_receive() const { return doc_.get_or_default("log_send_receive", "-"); }

    rjson::array locations() const { return doc_.get_or_empty_array("locations"); }

 protected:
    rjson::object doc_;

}; // class ServerSettings

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
