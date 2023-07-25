#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "new_dump_entry.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace openpower
{
namespace dump
{
namespace system
{

constexpr uint32_t INVALID_SOURCE_ID = 0xFFFFFFFF;
using NotifyIface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

/** @class Manager
 *  @brief System Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Notify DBus API
 */
class Manager :
    virtual public NotifyIface,
    virtual public phosphor::dump::Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = default;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] event - Dump manager sd_event loop.
     *  @param[in] path - Path to attach at.
     *  @param[in] baseEntryPath - Base path of the dump entry.
     */
    Manager(sdbusplus::bus_t& bus, const char* path,
            const std::string& baseEntryPath,
            phosphor::dump::host::HostTransport& hostTransport) :
        NotifyIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath),
        hostTransport(hostTransport)
    {}

    void restore() override
    {
        // TODO #2597  Implement the restore to restore the dump entries
        // after the service restart.
    }

    /** @brief Notify the system dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notify(uint32_t dumpId, uint64_t size) override;

    /** @brief Implementation for CreateDump
     *  Method to create a new system dump entry when user
     *  requests for a new system dump.
     *
     *  @return object_path - The path to the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

  private:
    phosphor::dump::host::HostTransport& hostTransport;

    phosphor::dump::BaseEntry* getInProgressEntry(uint32_t dumpId,
                                                  uint64_t size);
    bool isHostStateValid();
    std::string createEntry(uint32_t dumpId, uint64_t size,
                            phosphor::dump::OperationStatus status,
                            const std::string& originatorId,
                            phosphor::dump::OriginatorTypes originatorType,
                            phosphor::dump::host::HostTransport& hostTransport);
};

} // namespace system
} // namespace dump
} // namespace openpower
