#pragma once
#include "dump_manager.hpp"

#include <systemd/sd-event.h>
#include <unistd.h>

#include <errors_map.hpp>
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

/**
 * @brief Validates the dump type and returns the corresponding dump collection
 * type. If the dump type is empty, returns "user" by default. If the dump type
 * does not exist or does not match the specified category, an error is logged,
 *        and an InvalidArgument exception is thrown.
 *
 * @param[in] type - The dump type to validate.
 * @param[in] category - The category to match with the dump type.
 *
 * @return The corresponding dump collection type if the dump type and category
 * match an entry in the dumpTypeTable. If the type is empty or does not match
 * any entry, returns "user".
 *
 * @throws InvalidArgument - If the type does not match the specified category
 * or does not exist in the table.
 */
std::string validateDumpType(const std::string& type,
                             const std::string& category);

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
            lg2::error("An invalid input  passed for key: {KEY}", "KEY", key);
            elog<InvalidArgument>(Argument::ARGUMENT_NAME(key.c_str()),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }
    return T{};
}

/**
 * @brief Extracts the OriginatorId and OriginatorType properties from the
 * provided phosphor::dump::DumpCreateParams object.
 *
 * @param[in] params A phosphor::dump::DumpCreateParams object which is
 * essentially a map of property keys to property values associated with a dump.
 * @param[out] originatorId A string reference that will store the OriginatorId
 * obtained from the params object. OriginatorId provides the identifier for the
 * source that initiated the dump.
 * @param[out] originatorType A reference to OriginatorTypes enumerator that
 * will store the OriginatorType obtained from the params object. OriginatorType
 * provides the type or category of the source that initiated the dump.
 *
 * @throw InvalidArgument If an invalid value is provided in the params object,
 * an InvalidArgument error will be thrown.
 *
 * @note If the OriginatorId or OriginatorType is not provided in the params
 * object, this function will log a corresponding message and replace the
 * OriginatorType with a default value of OriginatorTypes::Internal.
 */
inline void extractOriginatorProperties(phosphor::dump::DumpCreateParams params,
                                        std::string& originatorId,
                                        OriginatorTypes& originatorType)
{
    using CreateParametersXYZ =
        sdbusplus::xyz::openbmc_project::Dump::server::Create::CreateParameters;

    std::string originatorIdKey =
        sdbusplus::xyz::openbmc_project::Dump::server::Create::
            convertCreateParametersToString(CreateParametersXYZ::OriginatorId);
    originatorId = extractParameter<std::string>(originatorIdKey, params);
    if (originatorId.empty())
    {
        lg2::info("OriginatorId is not provided");
    }

    std::string originatorTypeKey = sdbusplus::xyz::openbmc_project::Dump::
        server::Create::convertCreateParametersToString(
            CreateParametersXYZ::OriginatorType);
    std::string type = extractParameter<std::string>(originatorTypeKey, params);
    if (type.empty())
    {
        lg2::info("OriginatorType is not provided. Replacing the string "
                  "with the default value");
        originatorType = OriginatorTypes::Internal;
    }
    else
    {
        originatorType = sdbusplus::common::xyz::openbmc_project::common::
            OriginatedBy::convertOriginatorTypesFromString(type);
    }
}

/**
 * @brief This function fetches the dump type associated with a particular
 * error.
 *
 * @param[in] params The map of parameters passed as input.
 *
 * @return The dump type associated with the error. If no additional error type
 * is specified a generic elog type dump will be generated.
 */
inline std::string getErrorDumpType(phosphor::dump::DumpCreateParams& params)
{
    using CreateParameters =
        sdbusplus::xyz::openbmc_project::Dump::server::Create::CreateParameters;
    using DumpIntr = sdbusplus::common::xyz::openbmc_project::dump::Create;
    std::string dumpType = "elog";
    std::string errorType = extractParameter<std::string>(
        DumpIntr::convertCreateParametersToString(CreateParameters::ErrorType),
        params);
    const auto elogIt = errorMap.find(errorType);
    if (elogIt != errorMap.end())
    {
        dumpType = errorType;
    }
    return dumpType;
}

} // namespace dump
} // namespace phosphor
