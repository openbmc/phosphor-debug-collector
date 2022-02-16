#include "op_dump_util.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <fmt/core.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

#include <filesystem>

namespace openpower
{
namespace dump
{
namespace util
{

constexpr auto MP_REBOOT_FILE = "/run/openbmc/mpreboot@0";

using namespace phosphor::logging;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

void isOPDumpsEnabled()
{
    bool enabled = true;
    constexpr auto enable = "xyz.openbmc_project.Object.Enable";
    constexpr auto policy = "/xyz/openbmc_project/dump/system_dump_policy";
    constexpr auto property = "org.freedesktop.DBus.Properties";

    using disabled =
        sdbusplus::xyz::openbmc_project::Dump::Create::Error::Disabled;

    try
    {
        auto bus = sdbusplus::bus::new_default();

        auto service = phosphor::dump::getService(bus, policy, enable);

        auto method =
            bus.new_method_call(service.c_str(), policy, property, "Get");
        method.append(enable, "Enabled");
        auto reply = bus.call(method);
        std::variant<bool> v;
        reply.read(v);
        enabled = std::get<bool>(v);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>(
            fmt::format("Error({}) in getting dump policy, default is enabled",
                        e.what())
                .c_str());
        report<InternalFailure>();
    }

    if (!enabled)
    {
        log<level::ERR>("OpePOWER dumps are disabled, skipping");
        elog<disabled>();
    }
    log<level::INFO>("OpenPOWER dumps are enabled");
}

bool isInMpReboot()
{
    return std::filesystem::exists(MP_REBOOT_FILE);
}

bool isSystemDumpInProgress()
{
    using BiosBaseTableItem = std::pair<
        std::string,
        std::tuple<std::string, bool, std::string, std::string, std::string,
                   std::variant<int64_t, std::string>,
                   std::variant<int64_t, std::string>,
                   std::vector<std::tuple<
                       std::string, std::variant<int64_t, std::string>>>>>;
    using BiosBaseTable = std::vector<BiosBaseTableItem>;

    try
    {
        std::string dumpInProgress{};
        auto bus = sdbusplus::bus::new_default();

        auto retVal =
            phosphor::dump::readDBusProperty<std::variant<BiosBaseTable>>(
                bus, "xyz.openbmc_project.BIOSConfigManager",
                "/xyz/openbmc_project/bios_config/manager",
                "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");
        const auto baseBiosTable = std::get_if<BiosBaseTable>(&retVal);
        if (baseBiosTable == nullptr)
        {
            log<level::ERR>(
                "Util failed to read BIOSconfig property BaseBIOSTable");
            return false;
        }
        for (const auto& item : *baseBiosTable)
        {
            const auto attributeName = std::get<0>(item);
            auto attrValue = std::get<5>(std::get<1>(item));
            auto val = std::get_if<std::string>(&attrValue);
            if (val != nullptr && attributeName == "pvm_sys_dump_active")
            {
                dumpInProgress = *val;
                break;
            }
        }
        if (dumpInProgress.empty())
        {
            log<level::ERR>(
                "Util failed to read pvm_hmc_managed property value");
            return false;
        }
        if (dumpInProgress == "Enabled")
        {
            log<level::INFO>("A system dump is already in progress");
            return true;
        }
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Failed to read pvm_sys_dump_active ({})", ex.what())
                .c_str());
        return false;
    }

    log<level::INFO>("Another system dump is not in progress");
    return false;
}

int callback(sd_event_source*, const siginfo_t*, void*)
{
    // No specific action required in
    // the sd_event_add_child callback.
    return 0;
}
void captureDump(uint32_t dumpId, size_t allowedSize,
                 const std::string& inputDir, const std::string& packageDir,
                 const std::string& dumpPrefix,
                 const phosphor::dump::EventPtr& event)
{
    std::string idStr;
    try
    {
        idStr = std::to_string(dumpId);
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Dump capture: Error converting idto string");
        throw std::runtime_error(
            "Dump capture: Error converting dump id to string");
    }
    auto dumpTempPath = std::filesystem::path(inputDir) / idStr;

    pid_t pid = fork();
    if (pid == 0)
    {
        std::filesystem::path dumpPath(packageDir);
        dumpPath /= idStr;
        execl("/usr/bin/opdreport", "opdreport", "-d", dumpPath.c_str(), "-i",
              idStr.c_str(), "-s", std::to_string(allowedSize).c_str(), "-q",
              "-v", "-p", dumpTempPath.c_str(), "-n", dumpPrefix.c_str(),
              nullptr);

        // opdreport script execution is failed.
        auto error = errno;
        log<level::ERR>(
            fmt::format(
                "Dump capture: Error occurred during "
                "opdreport function execution, errno({}), dumpPrefix({}), "
                "dumpPath({}), dumpSourcePath({}), allowedSize({})",
                error, dumpPrefix.c_str(), dumpPath.c_str(),
                dumpTempPath.c_str(), allowedSize)
                .c_str());
        throw std::runtime_error(
            "Dump capture: Error occured during opdreport script execution");
    }
    else if (pid > 0)
    {
        auto rc = sd_event_add_child(event.get(), nullptr, pid,
                                     WEXITED | WSTOPPED, callback, nullptr);
        if (0 > rc)
        {
            // Failed to add to event loop
            log<level::ERR>(fmt::format("Dump capture: Error occurred during "
                                        "the sd_event_add_child call, rc({})",
                                        rc)
                                .c_str());
            throw std::runtime_error("Dump capture: Error occurred during the "
                                     "sd_event_add_child call");
        }
    }
    else
    {
        auto error = errno;
        log<level::ERR>(
            fmt::format("Dump capture: Error occurred during fork, errno({})",
                        error)
                .c_str());
        throw std::runtime_error("Dump capture: Error occurred during fork");
    }
}

} // namespace util
} // namespace dump
} // namespace openpower
