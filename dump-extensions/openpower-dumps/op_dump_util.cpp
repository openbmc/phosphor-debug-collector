#include "op_dump_util.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <unistd.h>

#include <com/ibm/Dump/Create/server.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

#include <filesystem>

namespace openpower
{
namespace dump
{
namespace util
{

constexpr auto MP_REBOOT_FILE = "/run/openbmc/mpreboot@0";

void isOPDumpsEnabled()
{
    auto enabled = true;
    constexpr auto enable = "xyz.openbmc_project.Object.Enable";
    constexpr auto policy = "/xyz/openbmc_project/dump/system_dump_policy";
    constexpr auto property = "org.freedesktop.DBus.Properties";

    using disabled =
        sdbusplus::xyz::openbmc_project::Dump::Create::Error::Disabled;

    try
    {
        auto bus = sdbusplus::bus::new_default();

        auto service = phosphor::dump::getService(bus, policy, enable);

        auto method = bus.new_method_call(service.c_str(), policy, property,
                                          "Get");
        method.append(enable, "Enabled");
        auto reply = bus.call(method);
        std::variant<bool> v;
        reply.read(v);
        enabled = std::get<bool>(v);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::error(
            "Error: {ERROR_MSG} in getting dump policy, default is enabled",
            "ERROR_MSG", e.what());
    }

    if (!enabled)
    {
        lg2::info("OpePOWER dumps are disabled, skipping");
        phosphor::logging::elog<disabled>();
    }
    lg2::info("OpenPOWER dumps are enabled");
}

bool isInMpReboot()
{
    auto inMpReboot = false;
    try
    {
        inMpReboot = std::filesystem::exists(MP_REBOOT_FILE);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        lg2::error(
            "Error in determing whether the file:{FILE} exists, error: {ERROR_MSG}",
            "FILE", MP_REBOOT_FILE, "ERROR_MSG", e.what());
    }
    return inMpReboot;
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
            lg2::error("Util failed to read BIOSconfig property BaseBIOSTable");
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
            lg2::error(
                "Util failed to read dump pvm_sys_dump_active  property value");
            return false;
        }
        if (dumpInProgress == "Enabled")
        {
            lg2::info("A system dump is already in progress");
            return true;
        }
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to read pvm_sys_dump_active error:{ERROR_MSG}",
                   "ERROR_MSG", ex.what());
        return false;
    }

    lg2::info("Another system dump is not in progress");
    return false;
}

void extractDumpCreateParams(const phosphor::dump::DumpCreateParams& params,
                             uint8_t dumpType, uint64_t& eid,
                             uint64_t& failingUnit)
{
    using namespace phosphor::logging;
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using CreateParameters =
        sdbusplus::com::ibm::Dump::server::Create::CreateParameters;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;

    eid = 0;
    failingUnit = 0;

    constexpr auto MAX_FAILING_UNIT = 0x20;
    constexpr auto MAX_ERROR_LOG_ID = 0xFFFFFFFF;

    // get error log id
    auto iter = params.find(
        sdbusplus::com::ibm::Dump::server::Create::
            convertCreateParametersToString(CreateParameters::ErrorLogId));
    if (iter == params.end())
    {
        log<level::ERR>("Required argument, error log id is not passed");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("ERROR_LOG_ID"),
                              Argument::ARGUMENT_VALUE("MISSING"));
    }

    if (!std::holds_alternative<uint64_t>(iter->second))
    {
        // Exception will be raised if the input is not uint64
        log<level::ERR>("An invalid error log id is passed, setting as 0");
        report<InvalidArgument>(Argument::ARGUMENT_NAME("ERROR_LOG_ID"),
                                Argument::ARGUMENT_VALUE("INVALID INPUT"));
    }

    eid = std::get<uint64_t>(iter->second);

    if (eid > MAX_ERROR_LOG_ID)
    {
        // An error will be logged if the error log id is larger than maximum
        // value and set the error log id as 0.
        log<level::ERR>(fmt::format("Error log id is greater than maximum({}) "
                                    "length, setting as 0, errorid({})",
                                    MAX_ERROR_LOG_ID, eid)
                            .c_str());
        report<InvalidArgument>(
            Argument::ARGUMENT_NAME("ERROR_LOG_ID"),
            Argument::ARGUMENT_VALUE(std::to_string(eid).c_str()));
        eid = 0;
    }

    if ((dumpType == SBE_DUMP_TYPE_HARDWARE) || (dumpType == SBE_DUMP_TYPE_SBE))
    {
        iter = params.find(sdbusplus::com::ibm::Dump::server::Create::
                               convertCreateParametersToString(
                                   CreateParameters::FailingUnitId));
        if (iter == params.end())
        {
            log<level::ERR>("Required argument, failing unit id is not passed");
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("FAILING_UNIT_ID"),
                                  Argument::ARGUMENT_VALUE("MISSING"));
        }

        if (!std::holds_alternative<uint64_t>(iter->second))
        {
            // Exception will be raised if the input is not uint64
            log<level::ERR>("An invalid failing unit id is passed ");
            report<InvalidArgument>(Argument::ARGUMENT_NAME("FAILING_UNIT_ID"),
                                    Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
        failingUnit = std::get<uint64_t>(iter->second);

        if (failingUnit > MAX_FAILING_UNIT)
        {
            log<level::ERR>(fmt::format("Invalid failing uint id: greater than "
                                        "maximum number({}): input({})",
                                        failingUnit, MAX_FAILING_UNIT)
                                .c_str());
            elog<InvalidArgument>(
                Argument::ARGUMENT_NAME("FAILING_UNIT_ID"),
                Argument::ARGUMENT_VALUE(std::to_string(failingUnit).c_str()));
        }
    }
}

} // namespace util
} // namespace dump
} // namespace openpower
