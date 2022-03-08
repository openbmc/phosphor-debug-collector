#include "op_dump_util.hpp"

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

} // namespace util
} // namespace dump
} // namespace openpower
