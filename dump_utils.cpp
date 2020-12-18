#include "dump_utils.hpp"

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{

using namespace phosphor::logging;

std::string getService(sdbusplus::bus::bus& bus, const std::string& path,
                       const std::string& interface)
{
    constexpr auto objectMapperName = "xyz.openbmc_project.ObjectMapper";
    constexpr auto objectMapperPath = "/xyz/openbmc_project/object_mapper";

    auto method = bus.new_method_call(objectMapperName, objectMapperPath,
                                      objectMapperName, "GetObject");

    method.append(path);
    method.append(std::vector<std::string>({interface}));

    std::vector<std::pair<std::string, std::vector<std::string>>> response;

    try
    {
        auto reply = bus.call(method);
        reply.read(response);
        if (response.empty())
        {
            log<level::ERR>("Error in mapper response for getting service name",
                            entry("PATH=%s", path.c_str()),
                            entry("INTERFACE=%s", interface.c_str()));
            return std::string{};
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in mapper method call",
                        entry("ERROR=%s", e.what()),
                        entry("PATH=%s", path.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
        return std::string{};
    }
    return response[0].first;
}

BootProgress getBootProgress()
{
    constexpr auto bootProgressInterface =
        "xyz.openbmc_project.State.Boot.Progress";
    // TODO Need to change host instance if multiple instead "0"
    constexpr auto hostStateObjPath = "/xyz/openbmc_project/state/host0";

    BootProgress bootProgessStage;

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto service = getService(bus, hostStateObjPath, bootProgressInterface);

        auto method =
            bus.new_method_call(service.c_str(), hostStateObjPath,
                                "org.freedesktop.DBus.Properties", "Get");

        method.append(bootProgressInterface, "BootProgress");

        auto reply = bus.call(method);

        using DBusValue_t =
            std::variant<std::string, bool, std::vector<uint8_t>,
                         std::vector<std::string>>;
        DBusValue_t propertyVal;

        reply.read(propertyVal);

        // BootProgress property type is string
        std::string bootPgs(std::get<std::string>(propertyVal));

        bootProgessStage = sdbusplus::xyz::openbmc_project::State::Boot::
            server::Progress::convertProgressStagesFromString(bootPgs);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("D-Bus call exception",
                        entry("OBJPATH=%s", hostStateObjPath),
                        entry("INTERFACE=%s", bootProgressInterface),
                        entry("EXCEPTION=%s", e.what()));
        throw std::runtime_error("Failed to get BootProgress stage");
    }
    catch (const std::bad_variant_access& e)
    {
        log<level::ERR>(
            "Exception raised while read BootProgress property value",
            entry("OBJPATH=%s", hostStateObjPath),
            entry("INTERFACE=%s", bootProgressInterface),
            entry("EXCEPTION=%s", e.what()));
        throw std::runtime_error("Failed to get BootProgress stage");
    }

    return bootProgessStage;
}

bool isHostRunning()
{
    // TODO #ibm-openbmc/dev/2858 Revisit the method for finding whether host
    // is running.
    BootProgress bootProgressStatus = phosphor::dump::getBootProgress();
    if ((bootProgressStatus == BootProgress::SystemInitComplete) ||
        (bootProgressStatus == BootProgress::OSStart) ||
        (bootProgressStatus == BootProgress::OSRunning))
    {
        return true;
    }
    return false;
}
} // namespace dump
} // namespace phosphor
