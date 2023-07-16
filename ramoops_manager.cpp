#include "config.h"

#include "ramoops_manager.hpp"

#include "dump_manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Dump/Create/common.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

#include <filesystem>
#include <set>

namespace phosphor
{
namespace dump
{
namespace ramoops
{

Manager::Manager(const std::string& filePath)
{
    namespace fs = std::filesystem;

    fs::path dir(filePath);
    if (!fs::exists(dir) || fs::is_empty(dir))
    {
        return;
    }

    std::vector<std::string> files;
    files.push_back(filePath);

    createHelper(files);
}

void Manager::createHelper(const std::vector<std::string>& files)
{
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
    constexpr auto DUMP_CREATE_IFACE = "xyz.openbmc_project.Dump.Create";

    auto b = sdbusplus::bus::new_default();
    auto mapper = b.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                    MAPPER_INTERFACE, "GetObject");
    mapper.append(BMC_DUMP_OBJPATH, std::set<std::string>({DUMP_CREATE_IFACE}));

    std::map<std::string, std::set<std::string>> mapperResponse;
    try
    {
        auto mapperResponseMsg = b.call(mapper);
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to parse dump create message, error: {ERROR}",
                   "ERROR", e);
        return;
    }
    if (mapperResponse.empty())
    {
        lg2::error("Error reading mapper response");
        return;
    }

    const auto& host = mapperResponse.cbegin()->first;
    auto m = b.new_method_call(host.c_str(), BMC_DUMP_OBJPATH,
                               DUMP_CREATE_IFACE, "CreateDump");
    phosphor::dump::DumpCreateParams params;
    using CreateParameters =
        sdbusplus::common::xyz::openbmc_project::dump::Create::CreateParameters;
    using DumpType =
        sdbusplus::common::xyz::openbmc_project::dump::Create::DumpType;
    using DumpIntr = sdbusplus::common::xyz::openbmc_project::dump::Create;
    params[DumpIntr::convertCreateParametersToString(
        CreateParameters::DumpType)] =
        DumpIntr::convertDumpTypeToString(DumpType::Ramoops);
    params[DumpIntr::convertCreateParametersToString(
        CreateParameters::FilePath)] = files.front();
    m.append(params);
    try
    {
        b.call_noreply(m);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to create ramoops dump, errormsg: {ERROR}", "ERROR",
                   e);
    }
}

} // namespace ramoops
} // namespace dump
} // namespace phosphor
