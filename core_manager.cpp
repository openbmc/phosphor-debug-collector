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

void watchCallback(UserMap fileInfo)
{
    vector<string> files;

    for (const auto& i : fileInfo)
    {
        // Get list of debug files.
        if (IN_CLOSE_WRITE == i.second)
        {
            files.push_back(i.first.string());
        }
    }
    createHelper(files);
}

void createHelper(vector<string>& files)
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
