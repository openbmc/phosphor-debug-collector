#include "dump_utils.hpp"

#include <fmt/core.h>

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
        (bootProgressStatus == BootProgress::OSStart) ||
        (bootProgressStatus == BootProgress::OSRunning) ||
        (bootProgressStatus == BootProgress::PCIInit))
    {
        return true;
    }
    return false;
}

// mapping of severity enum to severity interface
static std::unordered_map<PelSeverity, std::string> sevMap = {
    {PelSeverity::INFORMATIONAL,
     "xyz.openbmc_project.Logging.Entry.Level.Informational"},
    {PelSeverity::DEBUG, "xyz.openbmc_project.Logging.Entry.Level.Debug"},
    {PelSeverity::NOTICE, "xyz.openbmc_project.Logging.Entry.Level.Notice"},
    {PelSeverity::WARNING, "xyz.openbmc_project.Logging.Entry.Level.Warning"},
    {PelSeverity::CRITICAL, "xyz.openbmc_project.Logging.Entry.Level.Critical"},
    {PelSeverity::EMERGENCY,
     "xyz.openbmc_project.Logging.Entry.Level.Emergency"},
    {PelSeverity::ERROR, "xyz.openbmc_project.Logging.Entry.Level.Error"},
    {PelSeverity::ALERT, "xyz.openbmc_project.Logging.Entry.Level.Alert"}};

void createPEL(
    const std::unordered_map<std::string, std::string>& additionalData,
    const PelSeverity& sev, const std::string& errIntf)
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        constexpr auto loggerObjectPath = "/xyz/openbmc_project/logging";
        constexpr auto loggerCreateInterface =
            "xyz.openbmc_project.Logging.Create";
        constexpr auto loggerService = "xyz.openbmc_project.Logging";
        std::string pelSeverity =
            "xyz.openbmc_project.Logging.Entry.Level.Informational";
        auto itr = sevMap.find(sev);
        if (itr != sevMap.end())
            pelSeverity = itr->second;

        sd_bus* pSD_Bus = nullptr;
        ;
        sd_bus_default(&pSD_Bus);

        // Implies this is a call from Manager. Hence we need to make an async
        // call to avoid deadlock with Phosphor-logging.
        if (additionalData.empty() || additionalData.size() < 3)
        {
            log<level::WARNING>(
                fmt::format("User data map's size(({})) is not sufficient to "
                            "create a PEL message for dump delete/offload",
                            additionalData.size())
                    .c_str());
            throw std::runtime_error("Dump delete/offload PEL not created due "
                                     "to insufficient info passed");
        }
        auto itrToAdditionalData = additionalData.begin();
        auto retVal = sd_bus_call_method_async(
            pSD_Bus, nullptr, loggerService, loggerObjectPath,
            loggerCreateInterface, "Create", nullptr, nullptr, "ssa{ss}",
            errIntf.c_str(), pelSeverity.c_str(), 3,
            itrToAdditionalData->first.c_str(),
            itrToAdditionalData->second.c_str(),
            (++itrToAdditionalData)->first.c_str(),
            itrToAdditionalData->second.c_str(),
            (++itrToAdditionalData)->first.c_str(),
            itrToAdditionalData->second.c_str());

        if (retVal < 0)
        {
            log<level::ERR>("Error calling sd_bus_call_method_async",
                            entry("retVal=%d", retVal),
                            entry("MSG=%s", strerror(-retVal)));
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            "Error in calling creating PEL. Standard exception caught",
            entry("ERROR=%s", e.what()));
    }
}

} // namespace dump
} // namespace phosphor
