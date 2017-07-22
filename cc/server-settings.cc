#include <fstream>

#include "server-settings.hh"

// ----------------------------------------------------------------------

ServerSettings::~ServerSettings()
{
} // ServerSettings::~ServerSettings

// ----------------------------------------------------------------------

void ServerSettings::read(std::string aFilename)
{
    std::ifstream is{aFilename};
    if (!is)
        throw std::runtime_error("cannot read " + aFilename);
    rapidjson::IStreamWrapper wrapper{is};
    mDoc.ParseStream(wrapper);

} // ServerSettings::read

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
