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
namespace filewatch
{

using namespace phosphor::logging;
using namespace std;

void Manager::createHelper(const vector<string>& files)
{
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
    constexpr auto IFACE_INTERNAL("xyz.openbmc_project.Dump.Internal.Create");

    auto b = sdbusplus::bus::new_default();
    auto mapper = b.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                    MAPPER_INTERFACE, "GetObject");
    mapper.append(OBJ_INTERNAL, vector<string>({IFACE_INTERNAL}));

    auto mapperResponseMsg = b.call(mapper);
    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call");
        return;
    }

    map<string, vector<string>> mapperResponse;
    try
    {
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>(
            "Failed to parse dump create message", entry("ERROR=%s", e.what()),
            entry("REPLY_SIG=%s", mapperResponseMsg.get_signature()));
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
    m.append(dumpType, files);
    b.call_noreply(m);
}

} // namespace filewatch
} // namespace dump
} // namespace phosphor
