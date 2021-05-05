#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"

#include <filesystem>

namespace phosphor
{
namespace dump
{
namespace bmc_stored
{
using UserMap = phosphor::dump::inotify::UserMap;

using Watch = phosphor::dump::inotify::Watch;

/** @class Manager
 *  @brief Manager base class for locally stored dumps.
 */
class Manager : public phosphor::dump::Manager
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
     *  @param[in] dumpFilenameFormat - Format of dump filename in regex
     *  @param[in] maxDumpSize - Maximum size of the dump
     *  @param[in] minDumpSize - Minimum possible size of a usable dump.
     *  @param[in] allocatedSize - Total size allocated for the dumps
     */
    Manager(sdbusplus::bus::bus& bus, const EventPtr& event, const char* path,
            const std::string& baseEntryPath, const char* filePath,
            const std::string dumpFilenameFormat, const uint64_t maxDumpSize,
            const uint64_t minDumpSize, const uint64_t allocatedSize) :
        phosphor::dump::Manager(bus, path, baseEntryPath),
        dumpDir(filePath), eventLoop(event.get()),
        dumpWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            filePath,
            std::bind(std::mem_fn(
                          &phosphor::dump::bmc_stored::Manager::watchCallback),
                      this, std::placeholders::_1)),
        dumpFilenameFormat(dumpFilenameFormat), maxDumpSize(maxDumpSize),
        minDumpSize(minDumpSize), allocatedSize(allocatedSize)
    {}

    /** @brief Implementation of dump watch call back
     *  @param [in] fileInfo - map of file info  path:event
     */
    void watchCallback(const UserMap& fileInfo);

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    void restore() override;

  protected:
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

    /** @brief Calculate per dump allowed size based on the available
     *        size in the dump location.
     *  @returns dump size in kilobytes.
     */
    size_t getAllowedSize();

    /** @brief Path to the dump file*/
    std::string dumpDir;

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

  private:
    /** @brief Create Dump entry d-bus object
     *  @param[in] fullPath - Full path of the Dump file name
     */
    void createEntry(const std::filesystem::path& fullPath);

    /** @brief Remove specified watch object pointer from the
     *        watch map and associated entry from the map.
     *        @param[in] path - unique identifier of the map
     */
    void removeWatch(const std::filesystem::path& path);

    /** @brief Dump main watch object */
    Watch dumpWatch;

    /** @brief Child directory path and its associated watch object map
     *        [path:watch object]
     */
    std::map<std::filesystem::path, std::unique_ptr<Watch>> childWatchMap;

    std::string dumpFilenameFormat;

    uint64_t maxDumpSize;

    uint64_t minDumpSize;

    uint64_t allocatedSize;
};

} // namespace bmc_stored
} // namespace dump
} // namespace phosphor
