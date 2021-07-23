#include "config.h"

#include "core_manager.hpp"

#include <fmt/core.h>

#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>

#include <filesystem>
#include <regex>

namespace phosphor
{
namespace dump
{
namespace core
{

using namespace phosphor::logging;
using namespace std;

void Manager::watchCallback(const UserMap& fileInfo)
{
    vector<string> files;

    for (const auto& i : fileInfo)
    {
        std::filesystem::path file(i.first);
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

void Manager::createHelper(const vector<string>& files)
{
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
    constexpr auto IFACE_INTERNAL("xyz.openbmc_project.Dump.Internal.Create");
    constexpr auto APPLICATION_CORED =
        "xyz.openbmc_project.Dump.Internal.Create.Type.ApplicationCored";

    auto b = sdbusplus::bus::new_default();
    auto mapper = b.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                    MAPPER_INTERFACE, "GetObject");
    mapper.append(OBJ_INTERNAL, vector<string>({IFACE_INTERNAL}));

    map<string, vector<string>> mapperResponse;
    try
    {
        auto mapperResponseMsg = b.call(mapper);
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>(
            fmt::format("Failed to GetObject on Dump.Internal: {}", e.what())
                .c_str());
        return;
    }
    if (mapperResponse.empty())
    {
        log<level::ERR>("Error reading mapper response");
        return;
    }

    const auto& host = mapperResponse.cbegin()->first;
    auto m =
        b.new_method_call(host.c_str(), OBJ_INTERNAL, IFACE_INTERNAL, "Create");
    m.append(APPLICATION_CORED, files);
    try
    {
        b.call_noreply(m);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>(
            fmt::format("Failed to create dump: {}", e.what()).c_str());
    }
}

} // namespace core
} // namespace dump
} // namespace phosphor
