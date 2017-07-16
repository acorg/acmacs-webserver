#include "server-settings.hh"

#include "acmacs-base/read-file.hh"
#include "acmacs-base/json-importer.hh"
namespace jsi = json_importer;

// ----------------------------------------------------------------------

void ServerSettings::read_from_file(std::string aFilename)
{
    jsi::data<internal::Location> location_data = {
        {"location", jsi::field(&internal::Location::location)},
        {"files", jsi::field<internal::Location>(&internal::Location::files_ref)},
    };

    jsi::data<ServerSettings> settings_data = {
        {"host", jsi::field(&ServerSettings::host)},
        {"port", jsi::field(&ServerSettings::port)},
        {"mongodb_uri", jsi::field(&ServerSettings::mongodb_uri)},
        {"number_of_threads", jsi::field(&ServerSettings::number_of_threads)},
        {"certificate_chain_file", jsi::field(&ServerSettings::certificate_chain_file)},
        {"private_key_file", jsi::field(&ServerSettings::private_key_file)},
        {"tmp_dh_file", jsi::field(&ServerSettings::tmp_dh_file)},
        {"log_access", jsi::field(&ServerSettings::log_access)},
        {"log_error", jsi::field(&ServerSettings::log_error)},
        {"locations", jsi::field(&ServerSettings::locations_ref, location_data)},
    };

    jsi::import(acmacs_base::read_file(aFilename), *this, settings_data);

} // ServerSettings::read_from_file

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
