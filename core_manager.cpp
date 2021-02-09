#include "config.h"

#include "core_manager.hpp"

#include <experimental/filesystem>
#include <phosphor-logging/log.hpp>
#include <regex>
#include <sdbusplus/exception.hpp>

namespace phosphor
{
namespace dump
{
namespace core
{

using namespace phosphor::logging;
using namespace std;

void Manager::watchCallback(const filewatch::UserMap& fileInfo)
{
    vector<string> files;

    for (const auto& i : fileInfo)
    {
        namespace fs = std::experimental::filesystem;
        fs::path file(i.first);
        std::string name = file.filename();

        /*
          As per coredump source code systemd-coredump uses below format
          https://github.com/systemd/systemd/blob/master/src/coredump/coredump.c
          /var/lib/systemd/coredump/core.%s.%s." SD_ID128_FORMAT_STR “
          systemd-coredump also creates temporary file in core file path prior
          to actual core file creation. Checking the file name format will help
          to limit dump creation only for the new core files.
        */
        if ("core" == name.substr(0, name.find('.')))
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

} // namespace core
} // namespace dump
} // namespace phosphor
