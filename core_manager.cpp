#include <regex>
#include <experimental/filesystem>

#include <phosphor-logging/log.hpp>

#include "core_manager.hpp"
#include "config.h"

namespace phosphor
{
namespace dump
{
namespace core
{
namespace manager
{

using namespace phosphor::logging;
using namespace std;

void watchCallback(const UserMap& fileInfo)
{
    vector<string> files;

    for (const auto& i : fileInfo)
    {
        // Get list of debug files.
        if (IN_CLOSE_WRITE != i.second)
        {
            continue;
        }

        namespace fs = std::experimental::filesystem;
        fs::path file(i.first);
        std::string name = file.filename().c_str();

        /*
          As per coredump source code systemd-coredump uses below format
          https://github.com/systemd/systemd/blob/master/src/coredump/coredump.c
          /var/lib/systemd/coredump/core.%s.%s." SD_ID128_FORMAT_STR “
          systemd-coredump also creates temporary file in core file path prior
          to actual core file creation. Checking the file name format will help
          to limit dump creation only for the new core files.
        */
        std::regex file_regex("\\b(core.)([^ ]*)");
        std::smatch match;
        try
        {
            if (std::regex_match(name, match, file_regex))
            {
                files.push_back(file);
            }
        }
        catch (const std::regex_error& e)
        {
            //Log Error message and continue
            log<level::ERR>(e.what());
        }
    }

    if (!files.empty())
    {
        createHelper(files);
    }
}

void createHelper(const vector<string>& files)
{
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
    constexpr auto IFACE_INTERNAL("xyz.openbmc_project.Dump.Internal.Create");
    constexpr auto APPLICATION_CORED =
        "xyz.openbmc_project.Dump.Internal.Create.Type.ApplicationCored";

    auto b = sdbusplus::bus::new_default();
    auto mapper = b.new_method_call(
                      MAPPER_BUSNAME,
                      MAPPER_PATH,
                      MAPPER_INTERFACE,
                      "GetObject");
    mapper.append(OBJ_INTERNAL, vector<string>({IFACE_INTERNAL}));

    auto mapperResponseMsg = b.call(mapper);
    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call");
        return;
    }

    map<string, vector<string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);
    if (mapperResponse.empty())
    {
        log<level::ERR>("Error reading mapper response");
        return;
    }

    const auto& host = mapperResponse.cbegin()->first;
    auto m = b.new_method_call(
                 host.c_str(),
                 OBJ_INTERNAL,
                 IFACE_INTERNAL,
                 "Create");
    m.append(APPLICATION_CORED, files);
    b.call_noreply(m);
}

} // namespace manager
} // namespace core
} // namespace dump
} // namespace phosphor
