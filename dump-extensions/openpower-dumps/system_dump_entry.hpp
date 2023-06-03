#pragma once

#include "dump_entry.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "op_dump_util.hpp"
#include "xyz/openbmc_project/Dump/Entry/System/server.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower
{
namespace dump
{
namespace system
{

template <typename T>
using SystemDumpEntryIface = sdbusplus::server::object_t<T>;

using originatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

/**
 * @struct SystemDumpEntry
 * @brief Represents a system dump entry.
 *
 * This struct extends the generic system dump entry interface with
 * specific properties and methods that provide functionalities relevant
 * to system dump entries. It overrides the `deleteCheck` method to
 * include conditions specific to system dumps.
 *
 * @note The struct is derived from `SystemDumpEntryIfaces`, which is a
 * server object tied to the DBus interface for system dump entries.
 *
 * @property DumpName The name identifier for system dumps.
 * @property DumpIdentifier A unique numeric identifier for system dumps.
 *
 */

struct SystemDumpEntryExtn
{
    std::string dumpName = "System Dump";
    uint8_t dumpIdentifier = 3;

    SystemDumpEntryExtn() = default;
    SystemDumpEntryExtn(const SystemDumpEntryExtn&) = delete;
    SystemDumpEntryExtn& operator=(const SystemDumpEntryExtn&) = delete;
    SystemDumpEntryExtn(SystemDumpEntryExtn&&) = delete;
    SystemDumpEntryExtn& operator=(SystemDumpEntryExtn&&) = delete;
    ~SystemDumpEntryExtn() = default;

    bool deleteCheck(uint32_t dumpId, phosphor::dump::OperationStatus status)
    {
        // Skip the system dump delete if the dump is in progress
        // and in memory preserving reboot path
        if ((openpower::dump::util::isInMpReboot()) &&
            (status == phosphor::dump::OperationStatus::InProgress))
        {
            lg2::info(
                "Skip deleting system dump delete id: {ID} durng mp reboot",
                "ID", dumpId);
            return false;
        }
        return true;
    }
};

/**
 * @struct ResourceDumpEntry
 * @brief Represents a resource dump entry.
 *
 * This struct extends the generic resource dump entry interface with
 * specific properties and methods that provide functionalities relevant
 * to resource dump entries. Unlike `SystemDumpEntry`, it doesn't
 * contain a custom implementation for the `deleteCheck` method.
 *
 * @note The struct is derived from `ResourceDumpEntryIfaces`, which is a
 * server object tied to the DBus interface for resource dump entries.
 *
 * @property DumpName The name identifier for resource dumps.
 * @property DumpIdentifier A unique numeric identifier for resource dumps.
 *
 */

struct ResourceDumpEntryExtn
{
    std::string dumpName = "Resource Dump";
    uint8_t dumpIdentifier = 9;

    ResourceDumpEntryExtn() = default;
    ResourceDumpEntryExtn(const ResourceDumpEntryExtn&) = delete;
    ResourceDumpEntryExtn& operator=(const ResourceDumpEntryExtn&) = delete;
    ResourceDumpEntryExtn(ResourceDumpEntryExtn&&) = delete;
    ResourceDumpEntryExtn& operator=(ResourceDumpEntryExtn&&) = delete;
    ~ResourceDumpEntryExtn() = default;

    bool deleteCheck(uint32_t, phosphor::dump::OperationStatus)
    {
        return true;
    }
};

class Manager;

/** @class Entry
 *  @brief System Dump Entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
template <typename T, typename U>
class Entry :
    virtual public SystemDumpEntryIface<T>,
    virtual public phosphor::dump::Entry
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    ~Entry() = default;

    /** @brief Constructor for the Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] dumpId - Dump id.
     *  @param[in] timeStamp - Dump creation timestamp
     *             since the epoch.
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     *  @param[in] status - status  of the dump.
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, phosphor::dump::Manager& parent) :
        SystemDumpEntryIface<T>(bus, objPath.c_str(),
                                SystemDumpEntryIface<T>::action::defer_emit),
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                              std::string(), status, originatorId,
                              originatorType, parent)
    {
        SystemDumpEntryIface<T>::sourceDumpId(sourceId);
        // Emit deferred signal.
        this->openpower::dump::system::SystemDumpEntryIface<
            T>::emit_object_added();
    };

    /** @brief Constructor for the Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] dumpId - Dump id.
     *  @param[in] timeStamp - Dump creation timestamp
     *             since the epoch.
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     *  @param[in] vspStr- Input to host to generate the resource dump.
     *  @param[in] pwd - Password needed by host to validate the request.
     *  @param[in] status - status  of the dump.
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId,
          std::string vspStr, std::string pwd,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, phosphor::dump::Manager& parent) :
        SystemDumpEntryIface<T>(bus, objPath.c_str(),
                                SystemDumpEntryIface<T>::action::defer_emit),
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                              std::string(), status, originatorId,
                              originatorType, parent)
    {
        SystemDumpEntryIface<T>::sourceDumpId(sourceId);
        SystemDumpEntryIface<T>::vspString(vspStr);
        SystemDumpEntryIface<T>::password(pwd);
        // Emit deferred signal.
        this->openpower::dump::system::SystemDumpEntryIface<
            T>::emit_object_added();
    };

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump.
     */
    void initiateOffload(std::string uri) override
    {
        lg2::info(
            "Dump offload request id: {DUMP_ID} uri: {URI} source dumpid{SOURCE_ID}",
            "DUMP_ID", id, "URI", uri, "SOURCE_ID",
            SystemDumpEntryIface<T>::sourceDumpId());
        phosphor::dump::Entry::initiateOffload(uri);
        phosphor::dump::host::requestOffload(
            SystemDumpEntryIface<T>::sourceDumpId());
    }

    /** @brief Method to update an existing dump entry
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     */
    void update(uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId)
    {
        elapsed(timeStamp);
        size(dumpSize);
        SystemDumpEntryIface<T>::sourceDumpId(sourceId);
        // TODO: Handled dump failure case with
        // #bm-openbmc/2808
        status(OperationStatus::Completed);
        completedTime(timeStamp);
    }

    /**
     * @brief Delete host system dump and it entry dbus object
     */
    void delete_() override
    {
        auto srcDumpID = SystemDumpEntryIface<T>::sourceDumpId();
        auto dumpId = id;
        auto dumpPathOffLoadUri = offloadUri();
        auto hostRunning = phosphor::dump::isHostRunning();

        if (!u.deleteCheck(dumpId, status()))
        {
            return;
        }

        if (!dumpPathOffLoadUri.empty() && hostRunning)
        {
            lg2::error(
                "Dump offload in progress id: {ID} srcdumpid: {SOURCE_DUMP_ID}",
                "ID", dumpId, "SOURCE_DUMP_ID", srcDumpID);
            phosphor::logging::elog<
                sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
        }
        lg2::info("System dump delete id: {ID} srcdumpid: {SOURCE_DUMP_ID}",
                  "ID", dumpId, "SOURCE_DUMP_ID", srcDumpID);

        if (hostRunning && srcDumpID != INVALID_SOURCE_ID)
        {
            try
            {
                phosphor::dump::host::requestDelete(srcDumpID,
                                                    u.dumpIdentifier);
            }
            catch (const std::exception& e)
            {
                lg2::error(
                    "Error deleting dump from host id: {ID} srcdumpid: {SOURCE_DUMP_ID} error: {ERROR_MSG}",
                    "ID", dumpId, "SOURCE_DUMP_ID", srcDumpID, "ERROR_MSG",
                    e.what());
                phosphor::logging::elog<sdbusplus::xyz::openbmc_project::
                                            Common::Error::Unavailable>();
            }
        }

        phosphor::dump::Entry::delete_();
    }

  private:
    U u;
};

} // namespace system
} // namespace dump
} // namespace openpower
