#pragma once

#include "base_dump_manager.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"

#include <sdeventplus/source/child.hpp>

#include <filesystem>
#include <map>

namespace phosphor
{
namespace dump
{
namespace bmc
{

using UserMap = phosphor::dump::inotify::UserMap;

using Watch = phosphor::dump::inotify::Watch;
using ::sdeventplus::source::Child;

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API
 */
class Manager : public phosphor::dump::BaseManager
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
    Manager(sdbusplus::bus_t& bus, const EventPtr& event, const char* path,
            const std::string& baseEntryPath, const char* filePath) :
        phosphor::dump::BaseManager(bus, path),
        eventLoop(event.get()),
        dumpWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            filePath,
            std::bind(std::mem_fn(&phosphor::dump::bmc::Manager::watchCallback),
                      this, std::placeholders::_1)),
        dumpDir(filePath), baseEntryPath(baseEntryPath)
    {}

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
        createDump(phosphor::dump::DumpCreateParams params) override;

    void erase(uint32_t entryId) override;

    void deleteAll() override;

  private:
    /** @brief Create Dump entry d-bus object
     *  @param[in] fullPath - Full path of the Dump file name
     */
    void createEntry(const std::filesystem::path& fullPath);

    /** @brief Capture BMC Dump based on the Dump type.
     *  @param[in] type - Type of the dump to pass to dreport
     *  @param[in] path - An absolute path to the file
     *             to be included as part of Dump package.
     *  @return id - The Dump entry id number.
     */
    uint32_t captureDump(DumpTypes type, const std::string& path);

    /** @brief Remove specified watch object pointer from the
     *        watch map and associated entry from the map.
     *        @param[in] path - unique identifier of the map
     */
    void removeWatch(const std::filesystem::path& path);

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Dump main watch object */
    Watch dumpWatch;

    /** @brief Path to the dump file*/
    std::string dumpDir;

    /** @brief Flag to reject user intiated dump if a dump is in progress*/
    // TODO: https://github.com/openbmc/phosphor-debug-collector/issues/19
    static bool fUserDumpInProgress;

    /** @brief Child directory path and its associated watch object map
     *        [path:watch object]
     */
    std::map<std::filesystem::path, std::unique_ptr<Watch>> childWatchMap;

    /** @brief map of SDEventPlus child pointer added to event loop */
    std::map<pid_t, std::unique_ptr<Child>> childPtrMap;

    /** @brief Id of the last Dump entry */
    uint32_t lastEntryId;

    /** @brief Dump Entry dbus objects map based on entry id */
    std::map<uint32_t, std::unique_ptr<BaseEntry>> entries;

    /** @bried base object path for the entry object */
    std::string baseEntryPath;
};

} // namespace bmc
} // namespace dump
} // namespace phosphor
