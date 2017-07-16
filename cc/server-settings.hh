#pragma once

#include <string>

// ----------------------------------------------------------------------

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

    void read_from_file(std::string aFilename);

}; // class ServerSettings

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
