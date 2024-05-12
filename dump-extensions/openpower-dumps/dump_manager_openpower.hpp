#pragma once

#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "watch.hpp"

#include <com/ibm/Dump/Create/common.hpp>
#include <com/ibm/Dump/Notify/common.hpp>
#include <com/ibm/Dump/Notify/server.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/source/child.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace openpower::dump
{

using OpDumpIfaces = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::com::ibm::Dump::server::Notify>;

using UserMap = phosphor::dump::inotify::UserMap;

using Watch = phosphor::dump::inotify::Watch;
using ::sdeventplus::source::Child;

using NotifyDumpTypes = sdbusplus::common::com::ibm::dump::Notify::DumpType;
using OpDumpTypes = sdbusplus::common::com::ibm::dump::Create::DumpType;
/** @class Manager
 *  @brief OpenPOWER Dump manager implementation.
 *  @details A concrete implementation for the com.ibm.Dump.Notify and
 *           xyz.openbmc_project.Dump.Create DBus APIs
 */
class Manager :
    virtual public OpDumpIfaces,
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
     *  @param[in] baseEntryPath - Base path of the dump entry.
     *  @param[in] filePath The directory path where dump files are stored.
     */
    Manager(sdbusplus::bus_t& bus, const phosphor::dump::EventPtr& event,
            const char* path, const std::string& baseEntryPath,
            const char* filePath) :
        OpDumpIfaces(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath),
        eventLoop(event.get()),
        dumpWatch(eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
                  filePath,
                  [this](const UserMap& fileInfo) {
        for (const auto& [path, event] : fileInfo)
        {
            if (event == IN_CLOSE_WRITE && !std::filesystem::is_directory(path))
            {
                removeWatch(path.parent_path());
                updateEntry(path);
            }
            else if (event == IN_CREATE && std::filesystem::is_directory(path))
            {
                auto recursiveWatch = std::make_unique<Watch>(
                    eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE, EPOLLIN, path,
                    [this](const UserMap& recursiveFileInfo) {
                    for (const auto& [recursivePath, recursiveEvent] :
                         recursiveFileInfo)
                    {
                        if (recursiveEvent == IN_CLOSE_WRITE &&
                            !std::filesystem::is_directory(recursivePath))
                        {
                            removeWatch(recursivePath.parent_path());
                            updateEntry(recursivePath);
                        } // Here you might handle further nested directories if
                          // needed
                    }
                });
                childWatchMap.emplace(path, std::move(recursiveWatch));
            }
        }
    }),
        dumpDir(filePath)
    {}

    void restore() override
    {
        // TODO #2597  Implement the restore to restore the dump entries
        // after the service restart.
    }

    /** @brief Notify the system dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     *  @param[in] type - Type of the dump being notified
     *  @param[in] token - Token identifying the specific dump
     */
    void notifyDump(uint32_t dumpId, uint64_t size, NotifyDumpTypes type,
                    [[maybe_unused]] uint32_t token) override;

    /** @brief Implementation for CreateDump
     *  Method to create a new system dump entry when user
     *  requests for a new system dump.
     *
     *  @return object_path - The path to the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

    inline OpDumpTypes convertNotifyToCreateType(NotifyDumpTypes type)
    {
        using namespace phosphor::logging;
        using InvalidArgument =
            sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
        using Argument = xyz::openbmc_project::Common::InvalidArgument;
        switch (type)
        {
            case NotifyDumpTypes::System:
                return OpDumpTypes::System;
            case NotifyDumpTypes::Resource:
                return OpDumpTypes::Resource;
            default:
                lg2::error("An invalid type passed: {TYPE}", "TYPE", type);
                elog<InvalidArgument>(
                    Argument::ARGUMENT_NAME("TYPE"),
                    Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }

  private:
    /**
     * @brief Removes an inotify watch from the specified path.
     * @param[in] path The filesystem path from which to remove the watch.
     *
     * This method cleans up inotify watches that are no longer needed.
     */
    void removeWatch(const std::filesystem::path& path)
    {
        // Delete Watch entry from map.
        childWatchMap.erase(path);
    }

    /**
     * @brief Updates the dump entry based on the newly created or completed
     * dump file.
     * @param[in] fullPath The full path to the dump file.
     *
     * This method is called when a dump file is detected to be written
     * completely. It updates the corresponding dump entry with the new file
     * information.
     */
    void updateEntry(const std::filesystem::path& fullPath);

    /** @brief Pointer to the event loop used for asynchronous operations.*/
    phosphor::dump::EventPtr eventLoop;

    /** @brief Inotify watch object for monitoring the dump directory.*/
    Watch dumpWatch;

    /** @brief The directory path where dump files are stored and managed.*/
    std::string dumpDir;

    /**
     * @brief A map of child watch pointers to manage directory watches
     * recursively.
     *
     * This map holds unique pointers to Watch objects for managing directory
     * events recursively, ensuring that dumps created in nested directories are
     * tracked.
     */
    std::map<std::filesystem::path, std::unique_ptr<Watch>> childWatchMap;
};

} // namespace openpower::dump
