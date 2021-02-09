#include "config.h"

#include "ramoops_manager.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>

namespace phosphor
{
namespace dump
{
namespace ramoops
{

using namespace phosphor::logging;

void Manager::watchCallback(const UserMap& fileInfo)
{
    std::set<std::string> files;

    for (const auto& i : fileInfo)
    {
        if (IN_CLOSE_WRITE != i.second)
        {
            continue;
        }

        namespace fs = std::experimental::filesystem;
        fs::path file(i.first);
        files.emplace(file);
    }

    createHelper(files);
}

void Manager::createHelper(const std::set<std::string>& files)
{
    if (files.empty())
    {
        return;
    }

    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
    constexpr auto IFACE_INTERNAL("xyz.openbmc_project.Dump.Internal.Create");
    constexpr auto RAMOOPS =
        "xyz.openbmc_project.Dump.Internal.Create.Type.Ramoops";

    auto b = sdbusplus::bus::new_default();
    auto mapper = b.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                    MAPPER_INTERFACE, "GetObject");
    mapper.append(OBJ_INTERNAL, std::set<std::string>({IFACE_INTERNAL}));

    auto mapperResponseMsg = b.call(mapper);
    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call");
        return;
    }

    std::map<std::string, std::set<std::string>> mapperResponse;
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
    m.append(RAMOOPS, files);
    b.call_noreply(m);
}

} // namespace ramoops
} // namespace dump
} // namespace phosphor
