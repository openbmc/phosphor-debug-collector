#pragma once

#include "base_dump_entry.hpp"
#include "com/ibm/Dump/Entry/Resource/server.hpp"
#include "dump_entry.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "op_dump_util.hpp"
#include "system_dump_entry_helper.hpp"
#include "xyz/openbmc_project/Dump/Entry/System/server.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace dump
{
using SystemDump = sdbusplus::xyz::openbmc_project::Dump::Entry::server::System;
using ResourceDump = sdbusplus::com::ibm::Dump::Entry::server::Resource;

class Manager;

// Here is the specialization for T =
// sdbusplus::xyz::openbmc_project::Dump::Entry::server::System and U =
// DumpEntryHelper
template <>
class Entry<SystemDump, openpower::dump::DumpEntryHelper> :
    public DumpEntryIface<SystemDump>,
    public phosphor::dump::BaseEntry
{
    friend openpower::dump::DumpEntryHelper;

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
          OriginatorTypes originatorType, uint8_t transportId,
          phosphor::dump::Manager& parent) :
        DumpEntryIface<SystemDump>(
            bus, objPath.c_str(),
            DumpEntryIface<SystemDump>::action::defer_emit),
        phosphor::dump::BaseEntry(bus, objPath.c_str(), dumpId, timeStamp,
                                  dumpSize, std::string(), status, originatorId,
                                  originatorType, parent),
        dumpHelper(*this)
    {
        this->transportId = transportId;
        DumpEntryIface<SystemDump>::sourceDumpId(sourceId);
        // Emit deferred signal.
        this->phosphor::dump::DumpEntryIface<SystemDump>::emit_object_added();
    }

    void markComplete(uint64_t timeStamp, uint64_t dumpSize, uint32_t sourceId)
    {
        elapsed(timeStamp);
        size(dumpSize);
        sourceDumpId(sourceId);
        // TODO: Handled dump failure case with
        // #bm-openbmc/2808
        status(OperationStatus::Completed);
        completedTime(timeStamp);
    }
    void initiateOffload(std::string uri) override
    {
        dumpHelper.initiateOffload(
            id, DumpEntryIface<SystemDump>::sourceDumpId(), uri);
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
    openpower::dump::DumpEntryHelper dumpHelper;
    uint8_t transportId;
};

// Here is the specialization for T =
// sdbusplus::xyz::openbmc_project::Dump::Entry::server::System and U =
// DumpEntryHelper
template <>
class Entry<ResourceDump, openpower::dump::DumpEntryHelper> :
    public DumpEntryIface<ResourceDump>,
    public phosphor::dump::BaseEntry
{
    friend openpower::dump::DumpEntryHelper;

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
          OriginatorTypes originatorType, uint8_t transportId,
          phosphor::dump::Manager& parent) :
        DumpEntryIface<ResourceDump>(
            bus, objPath.c_str(),
            DumpEntryIface<ResourceDump>::action::defer_emit),
        phosphor::dump::BaseEntry(bus, objPath.c_str(), dumpId, timeStamp,
                                  dumpSize, std::string(), status, originatorId,
                                  originatorType, parent),
        dumpHelper(*this)
    {
        this->transportId = transportId;
        DumpEntryIface<ResourceDump>::sourceDumpId(sourceId);
        DumpEntryIface<ResourceDump>::vspString(vspStr);
        DumpEntryIface<ResourceDump>::password(pwd);
        // Emit deferred s
        this->phosphor::dump::DumpEntryIface<ResourceDump>::emit_object_added();
    }

    void markComplete(uint64_t timeStamp, uint64_t dumpSize, uint32_t sourceId)
    {
        elapsed(timeStamp);
        size(dumpSize);
        sourceDumpId(sourceId);
        // TODO: Handled dump failure case with
        // #bm-openbmc/2808
        status(OperationStatus::Completed);
        completedTime(timeStamp);
    }
    void initiateOffload(std::string uri) override
    {
        dumpHelper.initiateOffload(
            id, DumpEntryIface<ResourceDump>::sourceDumpId(), uri);
    }

    /**
     * @brief Delete host system dump and it entry dbus object
     */
    void delete_() override
    {
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
    openpower::dump::DumpEntryHelper dumpHelper;
    uint8_t transportId;
};

} // namespace dump
} // namespace phosphor
