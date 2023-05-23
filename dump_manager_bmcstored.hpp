#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"

#include <sdeventplus/source/child.hpp>

#include <filesystem>
#include <map>
namespace phosphor
{
namespace dump
{
namespace bmc_stored
{
using namespace phosphor::logging;
using UserMap = phosphor::dump::inotify::UserMap;

using Watch = phosphor::dump::inotify::Watch;
using ::sdeventplus::source::Child;

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
        eventLoop(event.get()), dumpDir(filePath),
        dumpWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            filePath,
            std::bind(std::mem_fn(
                          &phosphor::dump::bmc_stored::Manager::watchCallback),
                      this, std::placeholders::_1)),
        dumpFilenameFormat(dumpFilenameFormat), maxDumpSize(maxDumpSize),
        minDumpSize(minDumpSize), allocatedSize(allocatedSize)
    {}

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] event - Dump manager sd_event loop.
     *  @param[in] path - Path to attach at.
     *  @param[in] baseEntryPath - Base path for dump entry.
     *  @param[in] startingId - Starting id of the dump
     *  @param[in] filePath - Path where the dumps are stored.
     *  @param[in] dumpFilenameFormat - Format of dump filename in regex
     *  @param[in] maxDumpSize - Maximum size of the dump
     *  @param[in] minDumpSize - Minimum possible size of a usable dump.
     *  @param[in] allocatedSize - Total size allocated for the dumps
     */
    Manager(sdbusplus::bus::bus& bus, const EventPtr& event, const char* path,
            const std::string& baseEntryPath, uint32_t startingId,
            const char* filePath, const std::string dumpFilenameFormat,
            const uint64_t maxDumpSize, const uint64_t minDumpSize,
            const uint64_t allocatedSize) :
        phosphor::dump::Manager(bus, path, baseEntryPath, startingId),
        eventLoop(event.get()), dumpDir(filePath),
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

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

  protected:
    /** @brief Calculate per dump allowed size based on the available
     *        size in the dump location.
     *  @returns dump size in kilobytes.
     */
    size_t getAllowedSize();

    /** @brief Create a  Dump Entry Object
     *  @param[in] id - Id of the dump
     *  @param[in] objPath - Object path to attach to
     *  @param[in] timeStamp - Dump creation timestamp
     *             since the epoch.
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     *  @param[in] status - status of the dump.
     *  @param[in] parent - The dump entry's parent.
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     */
    virtual void createEntry(const uint32_t id, const std::string objPath,
                             const uint64_t ms, uint64_t fileSize,
                             const std::filesystem::path& file,
                             phosphor::dump::OperationStatus status,
                             std::string originatorId,
                             originatorTypes originatorType) = 0;

    /** @brief Path to the dump file*/
    std::string dumpDir;

    /** @brief map of SDEventPlus child pointer added to event loop */
    std::map<pid_t, std::unique_ptr<Child>> childPtrMap;

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

    /** @brief A regex based format for the filename */
    std::string dumpFilenameFormat;

    /** @brief Maximum possible size of a dump */
    uint64_t maxDumpSize;

    /** @brief Minimum viable dump size */
    uint64_t minDumpSize;

    /** @brief The total size allocated for a kind of dump */
    uint64_t allocatedSize;
};

} // namespace bmc_stored
} // namespace dump
} // namespace phosphor
