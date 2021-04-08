#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"

#include <filesystem>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc
{
namespace internal
{

class Manager;

} // namespace internal

using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

using UserMap = phosphor::dump::inotify::UserMap;

using Type =
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;

using Watch = phosphor::dump::inotify::Watch;

// Type to dreport type  string map
static const std::map<Type, std::string> TypeMap = {
    {Type::ApplicationCored, "core"},
    {Type::UserRequested, "user"},
    {Type::InternalFailure, "elog"},
    {Type::Checkstop, "checkstop"}};

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API
 */
class Manager : virtual public CreateIface,
                virtual public phosphor::dump::Manager
{
    friend class internal::Manager;

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
        dumpWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            filePath,
            std::bind(std::mem_fn(&phosphor::dump::bmc::Manager::watchCallback),
                      this, std::placeholders::_1)),
        dumpDir(filePath)
    {
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

  private:
    /** @brief Create Dump entry d-bus object
     *  @param[in] fullPath - Full path of the Dump file name
     */
    void createEntry(const std::filesystem::path& fullPath);

    /**  @brief Capture BMC Dump based on the Dump type.
     *  @param[in] type - Type of the Dump.
     *  @param[in] fullPaths - List of absolute paths to the files
     *             to be included as part of Dump package.
     *  @return id - The Dump entry id number.
     */
    uint32_t captureDump(Type type, const std::vector<std::string>& fullPaths);

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
    void removeWatch(const std::filesystem::path& path);

    /** @brief Calculate per dump allowed size based on the available
     *        size in the dump location.
     *  @returns dump size in kilobytes.
     */
    size_t getAllowedSize();

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Dump main watch object */
    Watch dumpWatch;

    /** @brief Path to the dump file*/
    std::string dumpDir;

    /** @brief Child directory path and its associated watch object map
     *        [path:watch object]
     */
    std::map<std::filesystem::path, std::unique_ptr<Watch>> childWatchMap;
};

} // namespace bmc
} // namespace dump
} // namespace phosphor
