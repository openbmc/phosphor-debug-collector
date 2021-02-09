#include "config.h"

#include "hostboot_dump_watch.hpp"

#include <experimental/filesystem>
#include <phosphor-logging/log.hpp>
#include <regex>
#include <sdbusplus/exception.hpp>

namespace openpower
{
namespace dump
{
namespace hostboot_watch
{

using namespace phosphor::logging;
using namespace std;

void Manager::watchCallback(const phosphor::dump::filewatch::UserMap& fileInfo)
{
    vector<string> files;

    for (const auto& i : fileInfo)
    {
        namespace fs = std::experimental::filesystem;
        fs::path file(i.first);
        std::string name = file.filename();

        if ("hbdump" == name.substr(0, name.find('.')))
        {
            // Consider only file name start with "core."
            files.push_back(file);
        }
    }

    if (!files.empty())
    {
        createHelper(files);
    }
}

} // namespace hostboot_watch
} // namespace dump
} // namespace openpower
