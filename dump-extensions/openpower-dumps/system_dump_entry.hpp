#pragma once

#include "host_dump_entry.hpp"

#include <xyz/openbmc_project/Dump/Entry/System/server.hpp>

namespace openpower::dump::host::system
{

using EntryIfaces = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::Entry::server::System>;

using SystemImpact =
    sdbusplus::common::xyz::openbmc_project::dump::entry::System::SystemImpact;
using HostResponse =
    sdbusplus::common::xyz::openbmc_project::dump::entry::System::HostResponse;

/** @class Entry
 *  @brief System Dump Entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
class Entry :
    public openpower::dump::host::Entry<system::Entry>,
    public EntryIfaces
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
          phosphor::dump::originatorTypes originatorType,
          phosphor::dump::Manager& parent);

    /** @brief Constructor for the Dump Entry Object with disruptive
     *         or non-disruptive system dump type.
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
     *  @param[in] sysImpact - Whether disruptive or not
     *  @param[in] usrChallenge - A user challenge for authentication
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId,
          phosphor::dump::OperationStatus status, std::string originatorId,
          phosphor::dump::originatorTypes originatorType,
          SystemImpact sysImpact, std::string usrChallenge,
          phosphor::dump::Manager& parent);

    /** @brief Constructor for creating a System dump entry with default values
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to.
     *  @param[in] dumpId - Unique identifier for the dump.
     *  @param[in] parent - Reference to the managing dump manager.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          phosphor::dump::Manager& parent);

    void setDumpRequestStatusImpl(HostResponse status)
    {
        dumpRequestStatus(status);
    }

  private:
    static constexpr auto TRANSPORT_DUMP_TYPE_IDENTIFIER = 3;
};
} // namespace openpower::dump::host::system
