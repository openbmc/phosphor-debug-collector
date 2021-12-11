#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

//#include <fmt/core.h>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

namespace phosphor
{
namespace dump
{
namespace faultlog
{

using namespace phosphor::logging;

//constexpr uint32_t INVALID_SOURCE_ID = 0xFFFFFFFF;
using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

/** @class Manager
 *  @brief System Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Notify DBus API
 */
class Manager :
    virtual public CreateIface,
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
     *  @param[in] baseEntryPath - Base path for dump entry.
     *  @param[in] filePath - Path where the dumps are stored.
     */
    Manager(sdbusplus::bus::bus& bus, const EventPtr& event, const char* path,
            const std::string& baseEntryPath, const char* filePath) :
        CreateIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath),
        eventLoop(event.get()),
        dumpDir(filePath)
    {}

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] event - Dump manager sd_event loop.
     *  @param[in] path - Path to attach at.
     *  @param[in] baseEntryPath - Base path of the dump entry.
     */
    Manager(sdbusplus::bus::bus& bus, const char* path,
            const std::string& baseEntryPath) :
        CreateIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath)
    {}

    void restore() override
    {
        // TODO #2597  Implement the restore to restore the dump entries
        // after the service restart.
        log<level::INFO>("dump_manager_faultlog restore");
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

    /** @brief sd_event_add_child callback
     *
     *  @param[in] s - event source
     *  @param[in] si - signal info
     *  @param[in] userdata - pointer to Watch object
     *
     *  @returns 0 on success, -1 on fail
     */
    static int callback(sd_event_source*, const siginfo_t*, void*)
    {
        log<level::INFO>("sd_event_add_child callback!");

        // No specific action required in
        // the sd_event_add_child callback.
        return 0;
    }
 private:
     /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

     /** @brief Path to the dump file*/
    std::string dumpDir;
};

} // namespace system
} // namespace dump
} // namespace openpower
