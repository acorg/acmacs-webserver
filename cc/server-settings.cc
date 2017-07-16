#include "server-settings.hh"

#include "acmacs-base/read-file.hh"
namespace jsi = json_importer;

// ----------------------------------------------------------------------

ServerSettings::~ServerSettings()
{
} // ServerSettings::~ServerSettings

// ----------------------------------------------------------------------

void ServerSettings::read_from_file(std::string aFilename)
{
    jsi::data<ServerSettings> settings_data;
    update_import_data(settings_data);

    jsi::import(acmacs_base::read_file(aFilename), *this, settings_data);

} // ServerSettings::read_from_file

// ----------------------------------------------------------------------

void ServerSettings::update_import_data(json_importer::data<ServerSettings>& aData)
{
    auto location_data = new jsi::data<internal::Location>{
        {"location", jsi::field(&internal::Location::location)},
        {"files", jsi::field<internal::Location>(&internal::Location::files_ref)},
    };

    aData.emplace("host", jsi::field(&ServerSettings::host));
    aData.emplace("port", jsi::field(&ServerSettings::port));
    aData.emplace("number_of_threads", jsi::field(&ServerSettings::number_of_threads));
    aData.emplace("certificate_chain_file", jsi::field(&ServerSettings::certificate_chain_file));
    aData.emplace("private_key_file", jsi::field(&ServerSettings::private_key_file));
    aData.emplace("tmp_dh_file", jsi::field(&ServerSettings::tmp_dh_file));
    aData.emplace("log_access", jsi::field(&ServerSettings::log_access));
    aData.emplace("log_error", jsi::field(&ServerSettings::log_error));
    aData.emplace("locations", jsi::field(&ServerSettings::locations_ref, *location_data));

} // update_import_data

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
