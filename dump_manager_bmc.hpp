#pragma once

#include "dump_internal.hpp"
#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"

#include <experimental/filesystem>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc
{
using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

using UserMap = phosphor::dump::inotify::UserMap;
using Type =
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;
namespace fs = std::experimental::filesystem;

using Watch = phosphor::dump::inotify::Watch;

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API
 */
class Manager : virtual public CreateIface, public phosphor::dump::Manager
{
    friend class phosphor::dump::internal::Manager;

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
     *  @param[in] dumpInternalMgr - Reference of internal dump manager.
     */
    Manager(sdbusplus::bus::bus& bus, const EventPtr& event, const char* path,
            const std::string& baseEntryPath, const char* filePath,
            phosphor::dump::internal::Manager& dumpInternalMgr) :
        CreateIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath),
        eventLoop(event.get()),
        dumpWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            filePath,
            std::bind(std::mem_fn(&phosphor::dump::bmc::Manager::watchCallback),
                      this, std::placeholders::_1)),
        dumpDir(filePath), dumpInternalMgr(dumpInternalMgr)
    {
        // Register the dump types handled by this dump manager.
        dumpInternalMgr.typeMap.emplace(
            Type::ApplicationCored,
            std::move(phosphor::dump::internal::DumpInfo("core", *this)));
        dumpInternalMgr.typeMap.emplace(
            Type::UserRequested,
            std::move(phosphor::dump::internal::DumpInfo("user", *this)));
        dumpInternalMgr.typeMap.emplace(
            Type::InternalFailure,
            std::move(phosphor::dump::internal::DumpInfo("elog", *this)));
        dumpInternalMgr.typeMap.emplace(
            Type::Checkstop,
            std::move(phosphor::dump::internal::DumpInfo("checkstop", *this)));
    }

    /** @brief Implementation of dump watch call back
     *  @param [in] fileInfo - map of file info  path:event
     */
    void watchCallback(const UserMap& fileInfo);

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    void restore() override;

    /** @brief Implementation for CreateDump
     *  Method to create a BMC dump entry when user requests for a new BMC dump
     *
     *  @return object_path - The object path of the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(std::map<std::string, std::string> params) override;

    /** @brief Calculate per dump allowed size based on the available
     *        size in the dump location.
     *  @returns dump size in kilobytes.
     */
    size_t getAllowedSize();

  private:
    /** @brief Create Dump entry d-bus object
     *  @param[in] fullPath - Full path of the Dump file name
     */
    void createEntry(const fs::path& fullPath);

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
        // No specific action required in
        // the sd_event_add_child callback.
        return 0;
    }
    /** @brief Remove specified watch object pointer from the
     *        watch map and associated entry from the map.
     *        @param[in] path - unique identifier of the map
     */
    void removeWatch(const fs::path& path);

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Dump main watch object */
    Watch dumpWatch;

    /** @brief Path to the dump file*/
    std::string dumpDir;

    /** @brief Child directory path and its associated watch object map
     *        [path:watch object]
     */
    std::map<fs::path, std::unique_ptr<Watch>> childWatchMap;

    /** @brief The manager for internal dump call implementation */
    phosphor::dump::internal::Manager& dumpInternalMgr;
};

} // namespace bmc
} // namespace dump
} // namespace phosphor
