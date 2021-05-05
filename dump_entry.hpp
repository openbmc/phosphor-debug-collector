#pragma once

#include "xyz/openbmc_project/Common/FilePath/server.hpp"
#include "xyz/openbmc_project/Common/OriginatedBy/server.hpp"
#include "xyz/openbmc_project/Common/Progress/server.hpp"
#include "xyz/openbmc_project/Dump/Entry/server.hpp"
#include "xyz/openbmc_project/Object/Delete/server.hpp"
#include "xyz/openbmc_project/Time/EpochTime/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <filesystem>

namespace phosphor
{
namespace dump
{

template <typename T>
using ServerObject = typename sdbusplus::server::object_t<T>;

// TODO Revisit whether sdbusplus::xyz::openbmc_project::Time::server::EpochTime
// still needed in dump entry since start time and completed time are available
// from sdbusplus::xyz::openbmc_project::Common::server::Progress
// #ibm-openbmc/2809
using EntryIfaces = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Common::server::OriginatedBy,
    sdbusplus::xyz::openbmc_project::Common::server::Progress,
    sdbusplus::xyz::openbmc_project::Dump::server::Entry,
    sdbusplus::xyz::openbmc_project::Object::server::Delete,
    sdbusplus::xyz::openbmc_project::Time::server::EpochTime>;

using OperationStatus =
    sdbusplus::xyz::openbmc_project::Common::server::Progress::OperationStatus;

using originatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

class Manager;

/** @class Entry
 *  @brief Base Dump Entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Entry DBus API
 */
class Entry : public EntryIfaces
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    ~Entry() = default;

    /** @brief Constructor for the Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] dumpId - Dump id.
     *  @param[in] timeStamp - Dump creation timestamp
     *             since the epoch.
     *  @param[in] dumpSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     *  @param[in] originId - Id of the originator of the dump
     *  @param[in] originType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize,
          const std::filesystem::path& file, OperationStatus dumpStatus,
          std::string originId, originatorTypes originType, Manager& parent) :
        EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::emit_no_signals),
        parent(parent), id(dumpId), file(file)
    {
        originatorId(originId);
        originatorType(originType);
        path(file);

        size(dumpSize);
        status(dumpStatus);

        // If the object is created after the dump creation keep
        // all same as timeStamp
        // if the object created before the dump creation, update
        // only the start time. Completed and elapsed time will
        // be updated once the dump is completed.
        if (dumpStatus == OperationStatus::Completed)
        {
            elapsed(timeStamp);
            startTime(timeStamp);
            completedTime(timeStamp);
        }
        else
        {
            elapsed(0);
            startTime(timeStamp);
            completedTime(0);
        }
    };

    /** @brief Delete this d-bus object.
     */
    void delete_() override;

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump
     */
    void initiateOffload(std::string uri) override
    {
        offloadUri(uri);
    }

    /** @brief Returns the dump id
     *  @return the id associated with entry
     */
    uint32_t getDumpId()
    {
        return id;
    }

    /** @brief Method to get the file handle of the dump
     *  @returns A Unix file descriptor to the dump file
     *  @throws sdbusplus::xyz::openbmc_project::Common::File::Error::Open on
     * failure to open the file
     *  @throws sdbusplus::xyz::openbmc_project::Common::Error::Unavailable if
     * the file string is empty
     */
    sdbusplus::message::unix_fd Entry::GetFileHandle() override
    {
        if (file.empty())
        {
            lg2::error("Failed to get file handle: File string is empty.");
            elog<xyz::openbmc_project::Common::Error::Unavailable>();
        }

        int fd = open(file.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd == -1)
        {
            lg2::error("Failed to open dump file: id: {ID} error: {ERROR}",
                       "ID", id, "ERROR", std::strerror(errno));
            elog<xyz::openbmc_project::Common::File::Error::Open>();
        }

        // Create a new Defer event source for closing this fd
        sdeventplus::Event event = sdeventplus::Event::get_default();
        auto eventSource = std::make_unique<sdeventplus::source::Defer>(
            event, [this, fd](auto& /*source*/) { closeFD(fd); });

        // Insert the file descriptor and corresponding event source into the
        // map
        fdCloseEventSources.emplace(fd, std::move(eventSource));

        return fd;
    }

    /** @brief Closes the file descriptor and removes the corresponding event
     * source.
     * @param[in] fd - The file descriptor to close.
     *
     * This function closes the given file descriptor and removes the
     * corresponding event source from the fdCloseEventSources map. The event
     * source's unique pointer is automatically deallocated when it is erased
     * from the map.
     */
    void Entry::closeFD(int fd)
    {
        close(fd);
        fdCloseEventSources.erase(fd);
    }

    /** @brief Method to update an existing dump entry, once the dump creation
     *  is completed this function will be used to update the entry which got
     *  created during the dump request.
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     */
    void update(uint64_t timeStamp, uint64_t fileSize,
                const std::filesystem::path& filePath)
    {
        elapsed(timeStamp);
        size(fileSize);
        // TODO: Handled dump failed case with #ibm-openbmc/2808
        status(OperationStatus::Completed);
        file = filePath;
        // TODO: serialization of this property will be handled with
        // #ibm-openbmc/2597
        completedTime(timeStamp);
    }

  protected:
    /** @brief This entry's parent */
    Manager& parent;

    /** @brief This entry's id */
    uint32_t id;

    /** @Dump file name */
    std::filesystem::path file;

    /**
     * @brief A map of file descriptors and corresponding event sources.
     */
    std::map<int, std::unique_ptr<sdeventplus::source::Defer>>
        fdCloseEventSources;
};

} // namespace dump
} // namespace phosphor
