#include "dump_utils.hpp"

#include <fmt/core.h>

#include <phosphor-logging/log.hpp>
#include <sdbusplus/async.hpp>

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
            log<level::ERR>(fmt::format("Error in mapper response for getting "
                                        "service name, PATH({}), INTERFACE({})",
                                        path, interface)
                                .c_str());
            return std::string{};
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        log<level::ERR>(fmt::format("Error in mapper method call, "
                                    "errormsg({}), PATH({}), INTERFACE({})",
                                    e.what(), path, interface)
                            .c_str());
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
    catch (const sdbusplus::exception_t& e)
    {
        log<level::ERR>(fmt::format("D-Bus call exception, OBJPATH({}), "
                                    "INTERFACE({}), EXCEPTION({})",
                                    hostStateObjPath, bootProgressInterface,
                                    e.what())
                            .c_str());
        throw std::runtime_error("Failed to get BootProgress stage");
    }
    catch (const std::bad_variant_access& e)
    {
        log<level::ERR>(
            fmt::format("Exception raised while read BootProgress property "
                        "value,  OBJPATH({}), INTERFACE({}), EXCEPTION({})",
                        hostStateObjPath, bootProgressInterface, e.what())
                .c_str());
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

using Unordered_Map = std::unordered_map<std::string_view, std::string_view>;

/**
 * @brief Log a new PEL message via co-routine
 *
 * @param[in] userDataMap - User information to be logged into the PEL
 * @param[in] pelSev - PEL severity (Informational by default)
 * @param[in] errIntf - D-Bus interface name.
 * @param[inout] ctx - The async proxy to D-Bus object
 * @return Returns a handle to the co-routine to be invoked by the caller
 **/
auto logPELViaCoRoutine(const Unordered_Map userDataMap,
                        const std::string pelSev, const std::string errIntf,
                        sdbusplus::async::context& ctx)
    -> sdbusplus::async::task<>
{
    constexpr auto loggerObjectPath = "/xyz/openbmc_project/logging";
    constexpr auto loggerCreateInterface = "xyz.openbmc_project.Logging.Create";
    constexpr auto loggerService = "xyz.openbmc_project.Logging";

    const auto systemd = sdbusplus::async::proxy()
                             .service(loggerService)
                             .path(loggerObjectPath)
                             .interface(loggerCreateInterface)
                             .preserve();

    log<level::INFO>("Going for writing PEL via co-routine");
    co_await systemd.call<>(ctx, "Create", errIntf, pelSev, userDataMap);
    log<level::INFO>("Writing PEL via co-routine is done now");

    // We are all done, so shutdown the server.
    ctx.request_stop();
    co_return;
}

void createPEL(sdbusplus::bus::bus&& dBus, const std::string& dumpFilePath,
               const std::string& dumpFileType, const int dumpId,
               const std::string& pelSev, const std::string& errIntf)
{
    try
    {
        constexpr auto dumpFileString = "File Name";
        constexpr auto dumpFileTypeString = "Dump Type";
        constexpr auto dumpIdString = "Dump ID";

        if (dBus.is_open())
        {
            log<level::INFO>("D-Bus object is open to the broker");
        }

        const Unordered_Map userDataMap = {
            {dumpIdString, std::to_string(dumpId)},
            {dumpFileString, dumpFilePath},
            {dumpFileTypeString, dumpFileType}};

        // Set up a proxy connection to D-Bus object
        sdbusplus::async::context ctx(std::move(dBus));

        // Implies this is a call from Manager. Hence we need to make an async
        // call to avoid deadlock with Phosphor-logging.
        log<level::INFO>("Spawning the co-routine");
        ctx.spawn(logPELViaCoRoutine(userDataMap, pelSev, errIntf, ctx));
        log<level::INFO>("Running the co-routine");
        ctx.run();
        log<level::INFO>("Running the co-routine done");
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in calling creating PEL. Exception caught",
                        entry("ERROR=%s", e.what()));
    }
}

} // namespace dump
} // namespace phosphor
