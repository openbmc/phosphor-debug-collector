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
namespace util
{
using namespace phosphor::logging;
using UserMap = phosphor::dump::inotify::UserMap;

using Watch = phosphor::dump::inotify::Watch;
using ::sdeventplus::source::Child;

/**
 * @class DumpHelper
 *
 * @brief This class provides helper methods for managing dumps.
 *
 * This class contains methods for creating dump entries, restoring dumps,
 * watching file changes, and managing dump sizes. It acts as a manager
 * to supervise the process of creating and managing dumps.
 */
class DumpHelper
{
  public:
    DumpHelper() = delete;
    DumpHelper(const DumpHelper&) = default;
    DumpHelper& operator=(const DumpHelper&) = delete;
    DumpHelper(DumpHelper&&) = delete;
    DumpHelper& operator=(DumpHelper&&) = delete;
    virtual ~DumpHelper() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] event - Dump manager sd_event loop.
     *  const char* filePath
     *  @param[in] dumpFilenameFormat - Format of dump filename in regex
     *  @param[in] maxDumpSize - Maximum size of the dump
     *  @param[in] minDumpSize - Minimum possible size of a usable dump.
     *  @param[in] allocatedSize - Total size allocated for the dumps
     */
    DumpHelper(sdbusplus::bus::bus& bus, const EventPtr& event,
               const char* filePath, const std::string dumpFilenameFormat,
               const uint64_t maxDumpSize, const uint64_t minDumpSize,
               const uint64_t allocatedSize, phosphor::dump::Manager* mgr) :
        bus(bus),
        eventLoop(event.get()), dumpDir(filePath),
        dumpWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            filePath,
            std::bind(
                std::mem_fn(&phosphor::dump::util::DumpHelper::watchCallback),
                this, std::placeholders::_1)),
        dumpFilenameFormat(dumpFilenameFormat), maxDumpSize(maxDumpSize),
        minDumpSize(minDumpSize), allocatedSize(allocatedSize), mgr(mgr)
    {}

    /** @brief Implementation of dump watch call back
     *  @param [in] fileInfo - map of file info  path:event
     */
    void watchCallback(const UserMap& fileInfo);

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    void restore();

    sdbusplus::bus::bus& bus;

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Calculate per dump allowed size based on the available
     *        size in the dump location.
     *  @returns dump size in kilobytes.
     */
    size_t getAllowedSize();

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

    phosphor::dump::Manager* mgr;
};

} // namespace util
} // namespace dump
} // namespace phosphor
