#pragma once

#include "config.h"

#include "dump_entry.hpp"
#include "dump_manager_remote.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Collection/DeleteAll/server.hpp"
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <experimental/filesystem>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace phosphor
{
namespace dump
{
namespace system
{

using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API and
 *  xyz::openbmc_project::Collection::server::DeleteAll.
 */
class Manager : public CreateIface, public phosphor::dump::remote::Manager
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
    Manager(sdbusplus::bus::bus& bus, const char* path,
            const std::string baseEntryPath) :
        CreateIface(bus, path),
        phosphor::dump::remote::Manager(bus, path, baseEntryPath)
    {
    }

    /** @brief Implementation for CreateDump
     *  Method to create Dump.
     *
     *  @return id - The Dump entry id number.
     */
    uint32_t createDump() override;
};

} // namespace system
} // namespace dump
} // namespace phosphor
