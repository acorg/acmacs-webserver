#pragma once

#include <string>
#include <vector>
#include <thread>
#include <fstream>

#include "acmacs-base/rjson-v2.hh"

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

    std::string_view host() const { return rjson::get_or(doc_, "host", ""); }
    size_t port() const { return rjson::get_or(doc_, "port", 0UL); }
    auto number_of_threads() const { return rjson::get_or(doc_, "number_of_threads", static_cast<size_t>(std::thread::hardware_concurrency())); }
    std::string_view certificate_chain_file() const { return rjson::get_or(doc_, "certificate_chain_file", ""); }
    std::string_view private_key_file() const { return rjson::get_or(doc_, "private_key_file", ""); }
    std::string_view tmp_dh_file() const { return rjson::get_or(doc_, "tmp_dh_file", ""); }
    std::string_view log_access() const { return rjson::get_or(doc_, "log_access", ""); }
    std::string_view log_error() const { return rjson::get_or(doc_, "log_error", ""); }
    std::string_view log_send_receive() const { return rjson::get_or(doc_, "log_send_receive", "-"); }

    const rjson::value& locations() const { return doc_["locations"]; }

 protected:
    rjson::value doc_;

}; // class ServerSettings

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
