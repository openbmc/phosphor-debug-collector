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
        createHelper();
    }
}


void Manager::createHelper()
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
        if (mapperResponse.empty())
        {
            log<level::ERR>("Error reading mapper response");
            throw std::runtime_error("Error reading mapper response");
        }

        const auto& host = mapperResponse.cbegin()->first;
        auto m =
            b.new_method_call(host.c_str(), OBJ_INTERNAL, IFACE_INTERNAL, "Create");
        m.append(APPLICATION_CORED, files);
        b.call_noreply(m);
        files.clear();
        retryCounter = 0;
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Failed to create dump: {}", e.what()).c_str());
        
        // core could be from phosphor-dump-manager, so retry after few seconds
        // so that dump manager service is back to create core dump
        if (!files.empty() && retryCounter < retryCount)
        {
            retryCounter++;
            retryTimer.restartOnce(retryTimeInSec);
            log<level::INFO>(
                fmt::format("Retry core dump: after 5 sec  counter {}",
                    retryCounter).c_str());
        }        
    }
}

void Manager::retryCoreDump()
{
    log<level::INFO>(
        fmt::format("retryCoreDump counter {} files size {}",
            retryCounter, files.size()).c_str());
    // check if another core dump did not pick the files before timer expiry
    if (!files.empty())
    {
        createHelper();
    }
}

} // namespace core
} // namespace dump
} // namespace phosphor
