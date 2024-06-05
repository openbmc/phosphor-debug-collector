#pragma once

#include "xyz/openbmc_project/Common/OriginatedBy/server.hpp"
#include "xyz/openbmc_project/Common/Progress/server.hpp"
#include "xyz/openbmc_project/Dump/Entry/server.hpp"
#include "xyz/openbmc_project/Object/Delete/server.hpp"
#include "xyz/openbmc_project/Time/EpochTime/server.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>

#include <filesystem>
#include <fstream>

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
     *  failure to open the file
     *  @throws sdbusplus::xyz::openbmc_project::Common::Error::Unavailable if
     *  the file string is empty
     */
    sdbusplus::message::unix_fd getFileHandle() override;

    /**
     * @brief Serialize the dump entry attributes to a file.
     *
     * @param[in] filePath - The path to the file where the entry will be
     *                       serialized.
     */
    virtual void serialize(const std::filesystem::path& filePath);

    /**
     * @brief Deserialize the dump entry attributes from a file.
     *
     * @param[in] filePath - The path to the file from where the entry
     *                       will be deserialized.
     */
    virtual void deserialize(const std::filesystem::path& filePath);

  protected:
    /** @brief This entry's parent */
    Manager& parent;

    /** @brief This entry's id */
    uint32_t id;

    /** @Dump file name */
    std::filesystem::path file;

  private:
    /** @brief Closes the file descriptor and removes the corresponding event
     *  source.
     *
     */
    void closeFD()
    {
        if (fdCloseEventSource)
        {
            close(fdCloseEventSource->first);
            fdCloseEventSource.reset();
        }
    }

    /* @brief A pair of file descriptor and corresponding event source. */
    std::optional<std::pair<int, std::unique_ptr<sdeventplus::source::Defer>>>
        fdCloseEventSource;
};

} // namespace dump
} // namespace phosphor
