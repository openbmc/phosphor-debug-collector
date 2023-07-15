#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace openpower
{
namespace dump
{
namespace system
{
// TODO #ibm-openbmc/issues/2859
// Revisit host transport impelementation
// This value is used to identify the dump in the transport layer to host,
constexpr auto TRANSPORT_DUMP_TYPE_IDENTIFIER = 3;

constexpr uint32_t INVALID_SOURCE_ID = 0xFFFFFFFF;
using NotifyIface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
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
            const std::string& baseEntryPath) :
        NotifyIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath),
        hostTransport(phosphor::dump::host::HostTransport::getInstance(
            TRANSPORT_DUMP_TYPE_IDENTIFIER))
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

    phosphor::dump::host::HostTransport* getHostTransport()
    {
        return hostTransport;
    }

  private:
    phosphor::dump::host::HostTransport* hostTransport;
};

} // namespace system
} // namespace dump
} // namespace openpower
