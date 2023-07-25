#pragma once

#include "dump_entry.hpp"
#include "dump_manager_system.hpp"
#include "host_dump_entry_handler.hpp"
#include "new_dump_entry.hpp"
#include "xyz/openbmc_project/Dump/Entry/System/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace phosphor
{
namespace dump
{
namespace new_
{
using SystemDump = sdbusplus::xyz::openbmc_project::Dump::Entry::server::System;

/**
 * @class Entry<SystemDump, DumpEntryHelper>
 * @brief Specialized derived class for SystemDump Entry.
 * @details This class provides a specialized concrete implementation for the
 * xyz.openbmc_project.Dump.Entry DBus API for SystemDump type, derived from the
 * BaseEntry class and instantiated with SystemDump as the dump type and
 * DumpEntryHelper as the helper class.
 *
 * This specialization provides the necessary DBus API for handling SystemDump
 * type dumps and utilizes DumpEntryHelper for additional functionalities.
 */
template <>
class Entry<SystemDump, openpower::dump::host::DumpEntryHelper> :
    public BaseEntry,
    public DumpEntryIface<SystemDump>
{
    friend openpower::dump::host::DumpEntryHelper;

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
          originatorTypes originatorType,
          phosphor::dump::host::HostTransport& hostTransport,
          phosphor::dump::BaseManager& parent) :
        BaseEntry(bus, objPath, dumpId, timeStamp, dumpSize, std::string(),
                  status, originatorId, originatorType, parent),
        DumpEntryIface<SystemDump>(
            bus, objPath.c_str(),
            DumpEntryIface<SystemDump>::action::emit_no_signals),
        helper(hostTransport, DumpTypes::SYSTEM)
    {
        sourceDumpId(sourceId);
        this->phosphor::dump::new_::DumpEntryIface<
            SystemDump>::emit_object_added();
    };

    /** @brief Deletes the dump from the host and from the D-Bus
     */
    void delete_() override
    {
        helper.delete_(id, sourceDumpId(), offloadUri());
        BaseEntry::delete_();
    }

    /** @brief Initiates the offload process for the dump
     *  @param[in] uri - URI to offload dump to
     */
    void initiateOffload(std::string uri) override
    {
        BaseEntry::initiateOffload(uri);
        helper.initiateOffload(sourceDumpId());
        offloaded(true);
    }

    /** @brief Returns a Unix file descriptor to the dump file
     *  @returns A Unix file descriptor to the dump file
     *  @throws sdbusplus::xyz::openbmc_project::Common::File::Error::Open on
     * failure to open the file
     *  @throws sdbusplus::xyz::openbmc_project::Common::Error::Unavailable if
     * the file string is empty
     */
    sdbusplus::message::unix_fd getFileHandle() override
    {
        return helper.getFileHandle(id);
    }

    /** @brief Updates an existing dump entry once the dump creation is
     * completed
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     */
    void markCompleteWithSourceId(uint64_t timeStamp, uint64_t fileSize,
                                  uint32_t sourceId)
    {
        elapsed(timeStamp);
        size(fileSize);
        sourceDumpId(sourceId);
        // TODO: Handled dump failure case with
        // #bm-openbmc/2808
        status(OperationStatus::Completed);
        completedTime(timeStamp);
    }

    /** @brief Method to update an existing dump entry, once the dump creation
     *  is completed this function will be used to update the entry which got
     *  created during the dump request.
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     */
    virtual void markComplete(uint64_t /*timeStamp*/, uint64_t /*fileSize*/,
                              const std::filesystem::path& /*filePath*/)
    {}

  private:
    openpower::dump::host::DumpEntryHelper helper;
};

} // namespace new_
} // namespace dump
} // namespace phosphor
