#pragma once
#include "config.h"

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"

#include <sdeventplus/source/child.hpp>

#include <regex>

namespace phosphor
{
namespace dump
{

using ::sdeventplus::source::Child;
using UserMap = phosphor::dump::inotify::UserMap;
using Watch = phosphor::dump::inotify::Watch;

/**
 * @class DumpStorageWatch
 * @brief Class to watch dump storage and handle events accordingly.
 *
 * The DumpStorageWatch class is designed to watch a specific directory where
 * dump files are stored. It uses inotify to asynchronously monitor the
 * filesystem for changes. When a change is detected, a callback function is
 * executed. This callback can then take appropriate actions such as creating a
 * new entry in the DumpManager for a newly created dump file.
 *
 * It can also restore dump entries from their persisted representations.
 */
template <typename T>
class DumpStorageWatch
{
  public:
    DumpStorageWatch() = delete;
    DumpStorageWatch(const DumpStorageWatch&) = delete;
    DumpStorageWatch& operator=(const DumpStorageWatch&) = delete;
    DumpStorageWatch(DumpStorageWatch&&) = delete;
    DumpStorageWatch& operator=(DumpStorageWatch&&) = delete;
    virtual ~DumpStorageWatch() = default;

    /**
     * @brief Constructs a new DumpStorageWatch object.
     *
     * @param[in] event - Event pointer for managing asynchronous events.
     * @param[in] filePath - Path of the file to be watched.
     * @param[in] dumpFilenameFormat - The format string for naming dump files.
     * @param[in] mgr - Reference to the DumpManager that manages dump entries.
     */
    DumpStorageWatch(const EventPtr& event, std::string_view filePath,
                     std::string_view dumpFilenameFormat, T& mgr) :
        eventLoop(event.get()),
        dumpWatch(eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
                  filePath,
                  [this](const UserMap& fileInfo) { watchCallback(fileInfo); }),
        dumpDir(filePath), dumpFilenameFormat(dumpFilenameFormat), mgr(mgr)
    {}

    /**
     * @brief Implementation of dump watch call back
     *
     * @param[in] fileInfo - map of file info path:event
     */
    void watchCallback(const UserMap& fileInfo)
    {
        for (const auto& [path, event] : fileInfo)
        {
            // For any new dump file create dump entry object
            // and associated inotify watch.
            if (event == IN_CLOSE_WRITE)
            {
                if (!std::filesystem::is_directory(path))
                {
                    // Don't require filename to be passed, as the path
                    // of dump directory is stored in the childWatchMap
                    removeWatch(path.parent_path());

                    // dump file is written now create D-Bus entry
                    createEntry(path);
                }
                else
                {
                    removeWatch(path);
                }
            }
            // Start inotify watch on newly created directory.
            else if ((event == IN_CREATE) &&
                     std::filesystem::is_directory(path))
            {
                auto watchObj = std::make_unique<Watch>(
                    eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE, EPOLLIN, path,
                    [this](const UserMap& info) { this->watchCallback(info); });

                childWatchMap.emplace(path, std::move(watchObj));
            }
        }
    }

    /**
     * @brief Construct dump d-bus objects from their persisted representations
     */
    void restore()
    {
        std::filesystem::path dir(dumpDir);
        if (!std::filesystem::exists(dir) || std::filesystem::is_empty(dir))
        {
            return;
        }

        // Dump file path: <DUMP_PATH>/<id>/<filename>
        for (const auto& p : std::filesystem::directory_iterator(dir))
        {
            auto idStr = p.path().filename().string();

            // Consider only directory's with dump id as name.
            // Note: As per design one file per directory.
            if ((std::filesystem::is_directory(p.path())) &&
                std::all_of(idStr.begin(), idStr.end(), ::isdigit))
            {
                // Extract the last four digits of the id string
                std::string lastFourDigits =
                    idStr.substr(std::max(int(idStr.size()) - 4, 0));
                uint16_t id = static_cast<uint16_t>(std::stoul(lastFourDigits));
                mgr.setLastEntryId(std::max(mgr.getLastEntryId(), id));
                auto fileIt = std::filesystem::directory_iterator(p.path());
                // Create dump entry d-bus object.
                if (fileIt != std::filesystem::end(fileIt))
                {
                    createEntry(fileIt->path());
                }
            }
        }
    }

  private:
    /**
     * @brief Create Dump entry d-bus object
     *
     * @param[in] fullPath - Full path of the Dump file name
     */
    void createEntry(const std::filesystem::path& file)
    {
        // Default Dump File Name format obmcdump_ID_EPOCHTIME.EXT
        std::regex file_regex(dumpFilenameFormat);

        std::smatch match;
        std::string name = file.filename();

        if (!((std::regex_search(name, match, file_regex)) &&
              (match.size() > 0)))
        {
            lg2::error("Invalid Dump file name, FILENAME: {FILENAME}",
                       "FILENAME", file);
            return;
        }

        uint64_t id = 0;
        uint64_t timestamp = 0;
        try
        {
            id = std::stoul(match[FILENAME_DUMP_ID_POS]);

            const uint64_t multiplier =
                1000000ULL;            // To convert to microseconds

            if (TIMESTAMP_FORMAT == 1) // Human-readable timestamp
            {
                timestamp = timeToEpoch(match[FILENAME_EPOCHTIME_POS]);
            }
            else
            {
                timestamp = std::stoull(match[FILENAME_EPOCHTIME_POS]);
            }

            timestamp *= multiplier;
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Unable to get id or timestamp from file name, FILENAME: {FILENAME} ERROR: {ERROR}",
                "FILENAME", file, "ERROR", e);
            return;
        }

        // If there is an existing entry update it or create a new one
        auto dumpEntry = mgr.getEntry(id);
        if (dumpEntry != nullptr)
        {
            dumpEntry->markComplete(timestamp, std::filesystem::file_size(file),
                                    file);
        }
        else
        {
            mgr.createEntry(id, timestamp, std::filesystem::file_size(file),
                            file, phosphor::dump::OperationStatus::Completed,
                            std::string(), OriginatorTypes::Internal);
        }
    }

    /**
     * @brief Remove specified watch object pointer from the watch map
     *        and associated entry from the map
     *
     * @param[in] path - unique identifier of the map
     */
    void removeWatch(const std::filesystem::path& path)
    {
        // Delete Watch entry from map.
        childWatchMap.erase(path);
    }

    /** sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** Dump main watch object */
    Watch dumpWatch;

    /** Path to the dump file */
    std::string dumpDir;

    /** Dump filename format */
    const std::string dumpFilenameFormat;

    /** Reference to DumpManager */
    T& mgr;

    /** Map of SDEventPlus child pointer added to event loop */
    std::map<pid_t, std::unique_ptr<Child>> childPtrMap;

    /** @brief Child directory path and its associated watch object map
     *        [path:watch object]
     */
    std::map<std::filesystem::path, std::unique_ptr<Watch>> childWatchMap;
};

} // namespace dump
} // namespace phosphor
