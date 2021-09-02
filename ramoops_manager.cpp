#include "config.h"

#include "ramoops_manager.hpp"

#include <fmt/core.h>

#include <sdbusplus/exception.hpp>

namespace phosphor
{
namespace dump
{
namespace ramoops
{

Manager::Manager(const std::string& filePath)
{
    std::vector<std::string> files;
    files.push_back(filePath);

    createHelper(files);
}

void Manager::createHelper(const std::vector<std::string>& files)
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

    std::map<std::string, std::set<std::string>> mapperResponse;
    try
    {
        auto mapperResponseMsg = b.call(mapper);
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        log<level::ERR>(
            fmt::format("Failed to parse dump create message, error({})",
                        e.what())
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
    m.append(RAMOOPS, files);
    try
    {
        b.call_noreply(m);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        log<level::ERR>(
            fmt::format("Failed to create ramoops dump, errormsg({})", e.what())
                .c_str());
    }
}

} // namespace ramoops
} // namespace dump
} // namespace phosphor
