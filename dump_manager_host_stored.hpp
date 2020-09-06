#pragma once

#include "config.h"

#include "dump_entry.hpp"
#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace phosphor
{
namespace dump
{
namespace host_stored
{

using NotifyIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

namespace fs = std::experimental::filesystem;

using Watch = phosphor::dump::inotify::Watch;

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API and
 *  xyz::openbmc_project::Collection::server::DeleteAll.
 */
class Manager : public NotifyIface, public phosphor::dump::Manager
{
    friend class Entry;

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
     */
    Manager(sdbusplus::bus::bus& bus, const char* path) :
        NotifyIface(bus, path), phosphor::dump::Manager(bus, path)
    {
    }

    void restore()
    {
    }

    /** @brief Notify the dump manager about creation of a new dump.
     *  @param[in] dumpType - Type of the Dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notify(NewDump::DumpType dumpType, uint32_t dumpId, uint64_t size);

};

} // namespace host_stored
} // namespace dump
} // namespace phosphor
