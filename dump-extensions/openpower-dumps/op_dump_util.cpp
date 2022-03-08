#include "op_dump_util.hpp"

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

} // namespace util
} // namespace dump
} // namespace openpower
