#include "op_dump_util.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <unistd.h>

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

BIOSAttrValueType readBIOSAttribute(const std::string& attrName)
{
    std::tuple<std::string, BIOSAttrValueType, BIOSAttrValueType> attrVal;
    auto bus = sdbusplus::bus::new_default();
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

bool isSystemDumpInProgress()
{
    try
    {
        auto dumpInProgress =
            std::get<std::string>(readBIOSAttribute("pvm_sys_dump_active"));
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
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        lg2::error("Failed to read pvm_sys_dump_active error:{ERROR}", "ERROR",
                   ex);
        return false;
    }

    lg2::info("Another system dump is not in progress");
    return false;
}

} // namespace util
} // namespace dump
} // namespace openpower
