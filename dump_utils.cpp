#include "dump_utils.hpp"

#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{

using namespace phosphor::logging;

std::string getService(sdbusplus::bus_t& bus, const std::string& path,
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
            lg2::error(
                "Error in mapper response for getting service name, PATH: {PATH}, INTERFACE: {INTERFACE}",
                "PATH", path, "INTERFACE", interface);
            return std::string{};
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Error in mapper method call, errormsg: {ERROR_MSG}, PATH: {PATH}, INTERFACE: {INTERFACE}",
            "ERROR_MSG", e, "PATH", path, "INTERFACE", interface);
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

        auto method = bus.new_method_call(service.c_str(), hostStateObjPath,
                                          "org.freedesktop.DBus.Properties",
                                          "Get");

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
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "D-Bus call exception, OBJPATH: {OBJ_PATH}, INTERFACE: {INTERFACE}, EXCEPTION: {EXCEPTION_MSG}",
            "OBJ_PATH", hostStateObjPath, "INTERFACE", bootProgressInterface,
            "EXCEPTION_MSG", e);
        throw std::runtime_error("Failed to get BootProgress stage");
    }
    catch (const std::bad_variant_access& e)
    {
        lg2::error(
            "Exception raised while read BootProgress property value, OBJPATH: {OBJ_PATH}, INTERFACE: {INTERFACE}, EXCEPTION: {EXCEPTION_MSG}",
            "OBJ_PATH", hostStateObjPath, "INTERFACE", bootProgressInterface,
            "EXCEPTION_MSG", e);
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
        (bootProgressStatus == BootProgress::SystemSetup) ||
        (bootProgressStatus == BootProgress::OSStart) ||
        (bootProgressStatus == BootProgress::OSRunning) ||
        (bootProgressStatus == BootProgress::PCIInit))
    {
        return true;
    }
    return false;
}
} // namespace dump
} // namespace phosphor
