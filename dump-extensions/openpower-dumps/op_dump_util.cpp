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

bool isOPDumpsEnabled(sdbusplus::bus::bus& bus)
{
    auto isEnabled = true;
    constexpr auto enable = "xyz.openbmc_project.Object.Enable";
    constexpr auto policy = "/xyz/openbmc_project/dump/system_dump_policy";
    constexpr auto property = "org.freedesktop.DBus.Properties";

    using sdBusDisabled =
        sdbusplus::xyz::openbmc_project::Dump::Create::Error::Disabled;

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
        lg2::error(
            "Error: {ERROR_MSG} in getting dump policy, default is enabled",
            "ERROR_MSG", e);
        isEnabled = false;
    }
    return isEnabled;
}

} // namespace util
} // namespace dump
} // namespace openpower
