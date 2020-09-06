#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace phosphor
{
namespace dump
{
namespace system
{

using NotifyIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

/** @class Manager
 *  @brief System Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Notify DBus API
 */
class Manager : virtual public NotifyIface,
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
    Manager(sdbusplus::bus::bus& bus, const char* path,
            const std::string& baseEntryPath) :
        NotifyIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath)
    {
    }

    void restore()
    {
        // TODO Implement the restore to restore the dump entries
        // after the service restart.
    }

    /** @brief Notify the system dump manager about creation of a new dump.
     *  @param[in] dumpType - Type of the Dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notify(NewDump::DumpType dumpType, uint32_t dumpId, uint64_t size);
};

} // namespace system
} // namespace dump
} // namespace phosphor
