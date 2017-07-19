#pragma once

#include <string>
#include <vector>
#include <thread>

#include "acmacs-base/rapidjson.hh"

// ----------------------------------------------------------------------

namespace internal
{
    class Location
    {
     public:
        inline Location() {}

        std::string location;
        std::vector<std::string> files;

        inline std::vector<std::string>& files_ref() { return files; } // for json_importer
    };
}

class ServerSettings
{
 public:
    inline ServerSettings() {}
    virtual ~ServerSettings();

    void read(std::string aFilename);

#define SS_FIELD(name, ret_type) inline ret_type name() const { return json_importer::get(mDoc, #name, ret_type{}); }
#define SS_FIELD_DEFAULT(name, defau) inline decltype(defau) name() const { return json_importer::get(mDoc, #name, defau); }

    SS_FIELD(host, std::string)
    SS_FIELD(port, size_t)
    SS_FIELD_DEFAULT(number_of_threads, std::thread::hardware_concurrency())
    SS_FIELD(certificate_chain_file, std::string)
    SS_FIELD(private_key_file, std::string)
    SS_FIELD(tmp_dh_file, std::string)
    SS_FIELD(log_access, std::string)
    SS_FIELD(log_error, std::string)

#undef SS_FIELD
#undef SS_FIELD_DEFAULT

    inline auto locations() const { return json_importer::get<rapidjson::Value::ConstArray>(mDoc, "locations"); }

 protected:
    rapidjson::Document mDoc;

    // std::vector<internal::Location> locations;

}; // class ServerSettings

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
