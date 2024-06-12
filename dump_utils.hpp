#pragma once
#include "dump_manager.hpp"
#include "dump_types.hpp"

#include <systemd/sd-event.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Dump/Create/common.hpp>
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
        sd_event_unref(event);
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
    CustomFd(int fd) : fd(fd) {}

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
 *
 * @throws sdbusplus::exception::SdBusError - If any D-Bus error occurs during
 * the call.
 **/
std::string getService(sdbusplus::bus_t& bus, const std::string& path,
                       const std::string& interface);

/**
 * @brief Read property value from the specified object and interface
 * @param[in] bus D-Bus handle
 * @param[in] service service which has implemented the interface
 * @param[in] object object having has implemented the interface
 * @param[in] intf interface having the property
 * @param[in] prop name of the property to read
 * @throws sdbusplus::exception::SdBusError if an error occurs in the dbus call
 * @return property value
 */
template <typename T>
T readDBusProperty(sdbusplus::bus_t& bus, const std::string& service,
                   const std::string& object, const std::string& intf,
                   const std::string& prop)
{
    T retVal{};
    try
    {
        auto properties = bus.new_method_call(service.c_str(), object.c_str(),
                                              "org.freedesktop.DBus.Properties",
                                              "Get");
        properties.append(intf);
        properties.append(prop);
        auto result = bus.call(properties);
        result.read(retVal);
    }
    catch (const std::exception& ex)
    {
        lg2::error(
            "Failed to get the property: {PROPERTY} interface: {INTERFACE} "
            "object path: {OBJECT_PATH} error: {ERROR} ",
            "PROPERTY", prop, "INTERFACE", intf, "OBJECT_PATH", object, "ERROR",
            ex);
        throw;
    }
    return retVal;
}

/**
 * @brief Get the state value
 *
 * @param[in] intf - Interface to get the value
 * @param[in] objPath - Object path of the service
 * @param[in] state - State name to get
 *
 * @return The state value as type T on successful retrieval.
 *
 * @throws sdbusplus::exception for D-Bus failures and std::bad_variant_access
 * for invalid value
 */
template <typename T>
T getStateValue(const std::string& intf, const std::string& objPath,
                const std::string& state)
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto service = getService(bus, objPath, intf);
        return std::get<T>(readDBusProperty<std::variant<T>>(
            bus, service, objPath, intf, state));
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "D-Bus call exception, OBJPATH: {OBJPATH}, "
            "INTERFACE: {INTERFACE}, PROPERTY: {PROPERTY}, error: {ERROR}",
            "OBJPATH", objPath, "INTERFACE", intf, "PROPERTY", state, "ERROR",
            e);
        throw;
    }
    catch (const std::bad_variant_access& e)
    {
        lg2::error("Exception raised while read state: {STATE} property "
                   "value,  OBJPATH: {OBJPATH}, INTERFACE: {INTERFACE}, "
                   "error: {ERROR}",
                   "STATE", state, "OBJPATH", objPath, "INTERFACE", intf,
                   "ERROR", e);
        throw;
    }
}

/**
 * @brief Get the host state
 *
 * @return HostState on success
 *
 * @throws std::runtime_error - If getting the state property fails
 */
inline HostState getHostState()
{
    constexpr auto hostStateInterface = "xyz.openbmc_project.State.Host";
    // TODO Need to change host instance if multiple instead "0"
    constexpr auto hostStateObjPath = "/xyz/openbmc_project/state/host0";
    return getStateValue<HostState>(hostStateInterface, hostStateObjPath,
                                    "CurrentHostState");
}

/**
 * @brief Get the host boot progress stage
 *
 * @return BootProgress on success
 *
 * @throws std::runtime_error - If getting the state property fails
 */
inline BootProgress getBootProgress()
{
    constexpr auto bootProgressInterface =
        "xyz.openbmc_project.State.Boot.Progress";
    // TODO Need to change host instance if multiple instead "0"
    constexpr auto hostStateObjPath = "/xyz/openbmc_project/state/host0";
    return getStateValue<BootProgress>(bootProgressInterface, hostStateObjPath,
                                       "BootProgress");
}

/**
 * @brief Check whether host is running
 *
 * @return true if the host running else false.
 *
 * @throws std::runtime_error - If getting the boot progress failed
 */
inline bool isHostRunning()
{
    // TODO #ibm-openbmc/dev/2858 Revisit the method for finding whether host
    // is running.
    BootProgress bootProgressStatus = getBootProgress();
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
        lg2::info("OriginatorId is not provided");
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
            lg2::error("An invalid originatorId passed. It should be a string, "
                       "errormsg: {ERROR}",
                       "ERROR", e);
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("ORIGINATOR_ID"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }

    iter = params.find(sdbusplus::xyz::openbmc_project::Dump::server::Create::
                           convertCreateParametersToString(
                               CreateParametersXYZ::OriginatorType));
    if (iter == params.end())
    {
        lg2::info("OriginatorType is not provided. Replacing the string "
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
            lg2::error("An invalid originatorType passed, errormsg: {ERROR}",
                       "ERROR", e);
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("ORIGINATOR_TYPE"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }
}

/**
 * @brief Check whether host is quiesced
 *
 * @return true if the host is quiesced else false.
 *
 * @throws std::runtime_error - If getting the state failed
 */
inline bool isHostQuiesced()
{
    return (getHostState() == HostState::Quiesced);
}

/** @brief Extract the dump create parameters
 *  @param[in] key - The name of the parameter
 *  @param[in] params - The map of parameters passed as input
 *
 *  @return On success, a std::optional containing the value of the parameter
 * (of type T). On failure (key not found in the map or the value is not of type
 * T), returns an empty std::optional.
 */
template <typename T>
T extractParameter(const std::string& key,
                   phosphor::dump::DumpCreateParams& params)
{
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;

    auto it = params.find(key);
    if (it != params.end())
    {
        const auto& [foundKey, variantValue] = *it;
        if (std::holds_alternative<T>(variantValue))
        {
            return std::get<T>(variantValue);
        }
        else
        {
            lg2::error("An invalid input passed for key: {KEY}", "KEY", key);
            elog<InvalidArgument>(Argument::ARGUMENT_NAME(key.c_str()),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }
    return T{};
}

/**
 * @brief This function fetches the dump type associated with a particular
 * error.
 *
 * @param[in] params The map of parameters passed as input.
 *
 * @return The dump type associated with the error.
 *
 * @throw std::invalid_argument If the dump type associated with the error
 * type is not found in the map.
 */
inline DumpTypes getErrorDumpType(phosphor::dump::DumpCreateParams& params)
{
    using CreateParameters =
        sdbusplus::xyz::openbmc_project::Dump::server::Create::CreateParameters;
    using DumpIntr = sdbusplus::common::xyz::openbmc_project::dump::Create;
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;

    std::string errorType = extractParameter<std::string>(
        DumpIntr::convertCreateParametersToString(CreateParameters::ErrorType),
        params);
    if (!isErrorTypeValid(errorType))
    {
        lg2::error("An invalid error type passed type: {ERROR_TYPE}",
                   "ERROR_TYPE", errorType);
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("ERROR_TYPE"),
                              Argument::ARGUMENT_VALUE(errorType.c_str()));
    }
    auto type = stringToDumpType(errorType);
    if (type.has_value())
    {
        return type.value();
    }

    // Ideally this should never happen, because if the error type is valid
    // it should be present in the dumpTypeToStringMap
    throw std::invalid_argument{"Dump type not found"};
}

/**
 * @brief Extracts the dump ID and timestamp from a BMC dump file name.
 *
 * @param[in] file The path to the dump file.
 *
 * @return A std::optional containing a tuple with the dump ID, timestamp
 * and size of the file if the extraction is successful, or std::nullopt
 * if the file name does not match the expected format.
 */
std::optional<std::tuple<uint32_t, uint64_t, uint64_t>>
    extractDumpDetails(const std::filesystem::path& file);

} // namespace dump
} // namespace phosphor
