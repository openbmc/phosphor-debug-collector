#pragma once

#include "dump_manager.hpp"

#include <fmt/core.h>
#include <fmt/format.h>
#include <systemd/sd-event.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>
#include <xyz/openbmc_project/State/Boot/Progress/server.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

#include <memory>

namespace phosphor
{
namespace dump
{

using BootProgress = sdbusplus::xyz::openbmc_project::State::Boot::server::
    Progress::ProgressStages;
using HostState =
    sdbusplus::xyz::openbmc_project::State::server::Host::HostState;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};
using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

/** @struct CustomFd
 *
 *  RAII wrapper for file descriptor.
 */
struct CustomFd
{
  private:
    /** @brief File descriptor */
    int fd = -1;

  public:
    CustomFd() = delete;
    CustomFd(const CustomFd&) = delete;
    CustomFd& operator=(const CustomFd&) = delete;
    CustomFd(CustomFd&&) = delete;
    CustomFd& operator=(CustomFd&&) = delete;

    /** @brief Saves File descriptor and uses it to do file operation
     *
     *  @param[in] fd - File descriptor
     */
    CustomFd(int fd) : fd(fd)
    {}

    ~CustomFd()
    {
        if (fd >= 0)
        {
            close(fd);
        }
    }

    int operator()() const
    {
        return fd;
    }
};

/**
 * @brief Get the bus service
 *
 * @param[in] bus - Bus to attach to.
 * @param[in] path - D-Bus path name.
 * @param[in] interface - D-Bus interface name.
 * @return the bus service as a string
 **/
std::string getService(sdbusplus::bus_t& bus, const std::string& path,
                       const std::string& interface);

/**
 * @brief Get the host state
 *
 * @return HostState on success
 *         Throw exception on failure
 *
 */
HostState getHostState();

/**
 * @brief Get the host boot progress stage
 *
 * @return BootProgress on success
 *         Throw exception on failure
 *
 */
BootProgress getBootProgress();

/**
 * @brief Get the host state value
 *
 * @param[in] intf - Interface to get the value
 * @param[in] objPath - Object path of the service
 * @param[in] state - State name to get
 *
 * @return The state value on success
 *         Throw exception on failure
 */
std::string getStateValue(const std::string& intf, const std::string& objPath,
                          const std::string& state);

/**
 * @brief Check whether host is running
 *
 * @return true if the host running else false.
 *         Throw exception on failure.
 */
bool isHostRunning();

inline void extractOriginatorProperties(phosphor::dump::DumpCreateParams params,
                                        std::string& originatorId,
                                        originatorTypes& originatorType)
{
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;
    using CreateParametersXYZ =
        sdbusplus::xyz::openbmc_project::Dump::server::Create::CreateParameters;

    auto iter = params.find(
        sdbusplus::xyz::openbmc_project::Dump::server::Create::
            convertCreateParametersToString(CreateParametersXYZ::OriginatorId));
    if (iter == params.end())
    {
        log<level::INFO>("OriginatorId is not provided");
    }
    else
    {
        try
        {
            originatorId = std::get<std::string>(iter->second);
        }
        catch (const std::bad_variant_access& e)
        {
            // Exception will be raised if the input is not string
            log<level::ERR>(
                fmt::format(
                    "An invalid  originatorId passed. It should be a string, "
                    "errormsg({})",
                    e.what())
                    .c_str());
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("ORIGINATOR_ID"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }

    iter = params.find(sdbusplus::xyz::openbmc_project::Dump::server::Create::
                           convertCreateParametersToString(
                               CreateParametersXYZ::OriginatorType));
    if (iter == params.end())
    {
        log<level::INFO>("OriginatorType is not provided. Replacing the string "
                         "with the default value");
        originatorType = originatorTypes::Internal;
    }
    else
    {
        try
        {
            std::string type = std::get<std::string>(iter->second);
            originatorType = sdbusplus::xyz::openbmc_project::Common::server::
                OriginatedBy::convertOriginatorTypesFromString(type);
        }
        catch (const std::bad_variant_access& e)
        {
            // Exception will be raised if the input is not string
            log<level::ERR>(fmt::format("An invalid originatorType passed, "
                                        "errormsg({})",
                                        e.what())
                                .c_str());
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("ORIGINATOR_TYPE"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }
}

/**
 * @brief Check whether host is quiesced
 *
 * @return true if the host is quiesced else false.
 *         Throw exception on failure.
 */
bool isHostQuiesced();

/**
 * @brief Read property value from the specified object and interface
 * @param[in] bus D-Bus handle
 * @param[in] service service which has implemented the interface
 * @param[in] object object having has implemented the interface
 * @param[in] intf interface having the property
 * @param[in] prop name of the property to read
 * @return property value
 */
template <typename T>
T readDBusProperty(sdbusplus::bus::bus& bus, const std::string& service,
                   const std::string& object, const std::string& intf,
                   const std::string& prop)
{
    using ::phosphor::logging::level;
    using ::phosphor::logging::log;
    T retVal{};
    try
    {
        auto properties =
            bus.new_method_call(service.c_str(), object.c_str(),
                                "org.freedesktop.DBus.Properties", "Get");
        properties.append(intf);
        properties.append(prop);
        auto result = bus.call(properties);
        result.read(retVal);
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Failed to get the property ({}) interface ({}) "
                        "object path ({}) error ({}) ",
                        prop.c_str(), intf.c_str(), object.c_str(), ex.what())
                .c_str());
        throw;
    }
    return retVal;
}

/**
 * @brief Create a new PEL message for dump Delete/Offload
 *
 * @param[in] dBus - Handle to D-Bus object
 * @param[in] dumpFilePath - Deleted dump file path/name
 * @param[in] dumpFileType - Deleted dump file type (BMC/Resource/System)
 * @param[in] dumpId - The dump ID
 * @param[in] pelSev - PEL severity (Informational by default)
 * @param[in] errIntf - D-Bus interface name.
 * @return Returns void
 */
void createPEL(sdbusplus::bus::bus& dBus, const std::string& dumpFilePath,
               const std::string& dumpFileType, const int dumpId,
               const std::string& pelSev, const std::string& errIntf);

} // namespace dump
} // namespace phosphor
