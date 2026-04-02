#include "op_dump_util.hpp"

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "op_dump_consts.hpp"

#include <unistd.h>

#include <com/ibm/Dump/Create/common.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/OriginatedBy/common.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Dump/Create/common.hpp>
#include <xyz/openbmc_project/Dump/Create/error.hpp>

#include <filesystem>

namespace openpower::dump::util
{

bool isOPDumpsEnabled(sdbusplus::bus_t& bus)
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

        auto method =
            bus.new_method_call(service.c_str(), policy, property, "Get");
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

BIOSAttrValueType readBIOSAttribute(const std::string& attrName,
                                    sdbusplus::bus_t& bus)
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

bool isSystemDumpInProgress(sdbusplus::bus_t& bus)
{
    try
    {
        auto dumpInProgress = std::get<std::string>(
            readBIOSAttribute("pvm_sys_dump_active", bus));
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

template <typename T>
inline std::optional<T> safeExtractParameter(
    const std::string& key, const phosphor::dump::DumpCreateParams& params)
{
    auto it = params.find(key);
    if (it == params.end())
    {
        return std::nullopt;
    }
    if (!std::holds_alternative<T>(it->second))
    {
        lg2::error("An invalid put is passed for {KEY}", "KEY", key);
        throwInvalidArgument(key, "INVALID_INPUT");
    }
    return std::get<T>(it->second);
}

openpower::dump::DumpParameters extractDumpParameters(
    const phosphor::dump::DumpCreateParams& params)
{
    using OpCreate = sdbusplus::common::com::ibm::dump::Create;
    using xyzCreate = sdbusplus::common::xyz::openbmc_project::dump::Create;

    // Extract mandatory parameters
    std::optional<std::string> type = safeExtractParameter<std::string>(
        OpCreate::convertCreateParametersToString(
            OpCreate::CreateParameters::DumpType),
        params);

    openpower::dump::OpDumpTypes dumpType;

    if (type && type.has_value())
    {
        dumpType = OpCreate::convertDumpTypeFromString(*type);
    }
    else
    {
        lg2::error("Dump type is missing in the list of arguments");
        throwInvalidArgument("DUMP_TYPE", "ARGUMENT_MISSING");
    }

    std::string originatorId =
        safeExtractParameter<std::string>(
            xyzCreate::convertCreateParametersToString(
                xyzCreate::CreateParameters::OriginatorId),
            params)
            .value_or("");

    std::optional<std::string> originatorTypeStr =
        safeExtractParameter<std::string>(
            xyzCreate::convertCreateParametersToString(
                xyzCreate::CreateParameters::OriginatorType),
            params);

    phosphor::dump::originatorTypes originatorType =
        phosphor::dump::originatorTypes::Internal;

    if (originatorTypeStr.has_value())
    {
        originatorType = sdbusplus::xyz::openbmc_project::Common::server::
            OriginatedBy::convertOriginatorTypesFromString(*originatorTypeStr);
    }

    // Extract optional parameters
    std::optional<std::string> vspString =
        safeExtractParameter<std::string>(
            OpCreate::convertCreateParametersToString(
                OpCreate::CreateParameters::VSPString),
            params)
            .value_or("");

    std::optional<std::string> userChallenge =
        safeExtractParameter<std::string>(
            OpCreate::convertCreateParametersToString(
                OpCreate::CreateParameters::Password),
            params);

    std::optional<uint64_t> eid = safeExtractParameter<uint64_t>(
        OpCreate::convertCreateParametersToString(
            OpCreate::CreateParameters::ErrorLogId),
        params);

    std::optional<uint64_t> fid = safeExtractParameter<uint64_t>(
        OpCreate::convertCreateParametersToString(
            OpCreate::CreateParameters::FailingUnitId),
        params);

    return {dumpType, vspString,    userChallenge, eid,
            fid,      originatorId, originatorType};
}

uint32_t getDumpIdPrefix(OpDumpTypes dumpType)
{
    switch (dumpType)
    {
        case OpDumpTypes::Hardware:
            return HARDWARE_DUMP_ID_PREFIX;
        case OpDumpTypes::Hostboot:
            return HOSTBOOT_DUMP_ID_PREFIX;
        case OpDumpTypes::SBE:
            return SBE_DUMP_ID_PREFIX;
        case OpDumpTypes::MemoryBufferSBE:
            return MSBE_DUMP_ID_PREFIX;
        case OpDumpTypes::System:
            return SYSTEM_DUMP_ID_PREFIX;
        case OpDumpTypes::Resource:
            return RESOURCE_DUMP_ID_PREFIX;
        default:
            lg2::error("unsupported {TYPE}", "TYPE", dumpType);
    }
    return 0xFF;
}

OpDumpTypes getDumpTypeFromId(uint32_t id)
{
    using namespace phosphor::logging;
    // Extract the highest byte as the prefix
    uint32_t prefix = id & DUMP_ID_PREFIX_MASK;

    switch (prefix)
    {
        case HARDWARE_DUMP_ID_PREFIX:
            return OpDumpTypes::Hardware;
        case HOSTBOOT_DUMP_ID_PREFIX:
            return OpDumpTypes::Hostboot;
        case SBE_DUMP_ID_PREFIX:
            return OpDumpTypes::SBE;
        case MSBE_DUMP_ID_PREFIX:
            return OpDumpTypes::MemoryBufferSBE;
        case SYSTEM_DUMP_ID_PREFIX:
            return OpDumpTypes::System;
        case RESOURCE_DUMP_ID_PREFIX:
            return OpDumpTypes::Resource;
        default:
            lg2::error("Unknown dump type");
    }

    return OpDumpTypes::System;
}

std::optional<OpDumpTypes> getDumpTypeFromPath(
    const std::filesystem::path& path)
{
    // Extract directory name (expected to be dump ID in hex)
    const auto idStr = path.filename().string();

    // Validate non-empty and hex
    if (idStr.empty() ||
        !std::all_of(idStr.begin(), idStr.end(),
                     [](unsigned char c) { return std::isxdigit(c); }))
    {
        return std::nullopt;
    }

    try
    {
        // Convert hex string to uint32_t
        uint32_t dumpId = static_cast<uint32_t>(std::stoul(idStr, nullptr, 16));

        return getDumpTypeFromId(dumpId);
    }
    catch (const std::exception&)
    {
        // Conversion failed
        lg2::error("Invalid dump ID in path: {PATH}, error: {ERROR}", "PATH",
                   path.string(), "ERROR", e.what());
        return std::nullopt;
    }
}

}
} // namespace openpower::dump::util
