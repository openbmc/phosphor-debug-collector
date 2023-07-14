#include "config.h"

#include "core_manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>

#include <filesystem>
#include <regex>

namespace phosphor
{
namespace dump
{
namespace core
{

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
    constexpr auto DUMP_CREATE_IFACE = "xyz.openbmc_project.Dump.Create";

    auto b = sdbusplus::bus::new_default();
    auto mapper = b.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                    MAPPER_INTERFACE, "GetObject");
    mapper.append(BMC_DUMP_OBJPATH, vector<string>({DUMP_CREATE_IFACE}));

    map<string, vector<string>> mapperResponse;
    try
    {
        auto mapperResponseMsg = b.call(mapper);
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to GetObject on Dump.Internal: {ERROR}", "ERROR", e);
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
        DumpIntr::convertDumpTypeToString(DumpType::ApplicationCored);
    params[DumpIntr::convertCreateParametersToString(
        CreateParameters::FilePath)] = files.front();
    m.append(params);
    try
    {
        b.call_noreply(m);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to create dump: {ERROR}", "ERROR", e);
    }
}

} // namespace core
} // namespace dump
} // namespace phosphor
