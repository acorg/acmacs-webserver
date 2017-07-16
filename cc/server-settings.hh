#pragma once

#include <string>
#include <vector>

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
    inline ServerSettings() : number_of_threads{0} {}

    std::string host;
    size_t port;
    std::string mongodb_uri;    // empty for default
    size_t number_of_threads;   // 0 - to autodetect
    std::string certificate_chain_file;
    std::string private_key_file;
    std::string tmp_dh_file;
    std::string log_access;
    std::string log_error;
    std::vector<internal::Location> locations;

    inline std::vector<internal::Location>& locations_ref() { return locations; } // for json_importer

    void read_from_file(std::string aFilename);

}; // class ServerSettings

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
