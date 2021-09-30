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

bool isOPDumpsEnabled(sdbusplus::bus::bus& bus)
{
    // Set isEnabled as true by default. In a field deployment, the system dump
    // feature is usually enabled to facilitate effective debugging in the event
    // of a failure. If due to some error, the settings service couldn't provide
    // the actual value, the system assumes that the dump is enabled.
    // This approach aligns with the principle of collecting as much data as
    // possible for debugging in case of a system failure.
    auto isEnabled = true;

    constexpr auto enable = "xyz.openbmc_project.Object.Enable";
    constexpr auto policy = "/xyz/openbmc_project/dump/system_dump_policy";
    constexpr auto property = "org.freedesktop.DBus.Properties";

    try
    {
        auto service = phosphor::dump::getService(bus, policy, enable);

        auto method = bus.new_method_call(service.c_str(), policy, property,
                                          "Get");
        method.append(enable, "Enabled");
        auto reply = bus.call(method);
        std::variant<bool> v;
        reply.read(v);
        isEnabled = std::get<bool>(v);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::error("Error: {ERROR} in getting dump policy, default is enabled",
                   "ERROR", e);
    }
    return isEnabled;
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
        lg2::error("Error in determing whether the file:{FILE} exists,"
                   "error: {ERROR_MSG}",
                   "FILE", MP_REBOOT_FILE, "ERROR_MSG", e.what());
    }
    return inMpReboot;
}

BIOSAttrValueType readBIOSAttribute(const std::string& attrName,
                                    sdbusplus::bus::bus& bus)
{
    std::tuple<std::string, BIOSAttrValueType, BIOSAttrValueType> attrVal;
    auto method = bus.new_method_call(
        "xyz.openbmc_project.BIOSConfigManager",
        "/xyz/openbmc_project/bios_config/manager",
        "xyz.openbmc_project.BIOSConfig.Manager", "GetAttribute");
    method.append(attrName);
    try
    {
        auto result = bus.call(method);
        result.read(std::get<0>(attrVal), std::get<1>(attrVal),
                    std::get<2>(attrVal));
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::error("Failed to read BIOS Attribute: {ATTRIBUTE_NAME}",
                   "ATTRIBUTE_NAME", attrName);
        throw;
    }
    return std::get<1>(attrVal);
}

bool isSystemDumpInProgress(sdbusplus::bus::bus& bus)
{
    try
    {
        auto dumpInProgress = std::get<std::string>(
            readBIOSAttribute("pvm_sys_dump_active"), bus);
        if (dumpInProgress == "Enabled")
        {
            lg2::info("A system dump is already in progress");
            return true;
        }
    }
    catch (const std::bad_variant_access& ex)
    {
        lg2::error("Failed to read pvm_sys_dump_active property value due "
                   "to bad variant access error:{ERROR}",
                   "ERROR", ex);
        return false;
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to read pvm_sys_dump_active error:{ERROR}", "ERROR",
                   ex);
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
