#pragma once

#include "base_dump_entry.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "op_dump_util.hpp"
#include "xyz/openbmc_project/Dump/Entry/System/server.hpp"
#include "com/ibm/Dump/Entry/Resource/server.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower
{
namespace dump
{

template <typename T>
using DumpEntryIface = sdbusplus::server::object_t<T>;

using OriginatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

class Manager;


/** @class Entry
 *  @brief System Dump Entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
template <typename T>
class Entry : public DumpEntryIface<T>, public phosphor::dump::BaseEntry
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
          OriginatorTypes originatorType, uint8_t transportId, phosphor::dump::Manager& parent) :
          DumpEntryIface<T>(
            bus, objPath.c_str(), DumpEntryIface<T>::action::defer_emit),
        phosphor::dump::BaseEntry(
            bus, objPath.c_str(), dumpId, timeStamp, dumpSize, std::string(),
            status, originatorId, originatorType, parent)
    {
        this->transportId = transportId;
        DumpEntryIface<T>::sourceDumpId(sourceId);
        // Emit deferred signal.
        this->openpower::dump::DumpEntryIface<T>::emit_object_added();
    }

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
          OriginatorTypes originatorType,uint8_t transportId,  phosphor::dump::Manager& parent) :
        DumpEntryIface<T>(
            bus, objPath.c_str(), DumpEntryIface<T>::action::defer_emit),
        phosphor::dump::BaseEntry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                              std::string(), status, originatorId,
                              originatorType, parent)
    {
        this->transportId = transportId;
        DumpEntryIface<T>::sourceDumpId(sourceId);
        DumpEntryIface<T>::vspString(vspStr);
        DumpEntryIface<T>::password(pwd);
        // Emit deferred s
        this->openpower::dump::DumpEntryIface<T>::emit_object_added();
    }

    void initiateOffload(std::string uri) override
    {
        lg2::info(
            "Dump offload request id: {DUMP_ID} uri: {URI} source dumpid{SOURCE_ID}",
            "DUMP_ID", id, "URI", uri, "SOURCE_ID",
            DumpEntryIface<T>::sourceDumpId());
        phosphor::dump::BaseEntry::initiateOffload(uri);
        phosphor::dump::host::requestOffload(
            DumpEntryIface<T>::sourceDumpId());
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
        Entry<T>::sourceDumpId(sourceId);
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
        auto srcDumpID = Entry<T>::sourceDumpId();
        auto dumpId = id;
        auto dumpPathOffLoadUri = offloadUri();
        auto hostRunning = phosphor::dump::isHostRunning();

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
                phosphor::dump::host::requestDelete(srcDumpID, transportId);
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

        phosphor::dump::BaseEntry::delete_();
    }

     /** @brief Method to get the file handle of the dump
     *  @returns A Unix file descriptor to the dump file
     *  @throws sdbusplus::xyz::openbmc_project::Common::File::Error::Open on
     *  failure to open the file
     *  @throws sdbusplus::xyz::openbmc_project::Common::Error::Unavailable if
     *  the file string is empty
     */
    sdbusplus::message::unix_fd getFileHandle() override
    {
	 return 0;
    }

  private:
    uint8_t transportId;
};

} // namespace dump
} // namespace openpower
