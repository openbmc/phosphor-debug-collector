#include "config.h"

#include "kernel_manager.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>

namespace phosphor
{
namespace dump
{
namespace kernel
{

using namespace phosphor::logging;
using namespace std;

void Manager::watchCallback(const UserMap& fileInfo)
{
    vector<string> files;

    for (const auto& i : fileInfo)
    {
        if (IN_CLOSE_WRITE != i.second)
        {
            continue;
        }

        namespace fs = std::experimental::filesystem;
        fs::path file(i.first);
        std::string name = file.filename();

        bool isExsit = false;
        for (size_t i = 0; i < files.size(); i++)
        {
            if (files[i] == file)
            {
                isExsit = true;
                break;
            }
        }

        if (!isExsit)
        {
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
    constexpr auto KERNEL_OOPS =
        "xyz.openbmc_project.Dump.Internal.Create.Type.KernelOops";

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
    m.append(KERNEL_OOPS, files);
    b.call_noreply(m);
}

} // namespace kernel
} // namespace dump
} // namespace phosphor
