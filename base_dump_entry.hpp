#pragma once

#include "xyz/openbmc_project/Common/OriginatedBy/server.hpp"
#include "xyz/openbmc_project/Common/Progress/server.hpp"
#include "xyz/openbmc_project/Dump/Entry/server.hpp"
#include "xyz/openbmc_project/Object/Delete/server.hpp"
#include "xyz/openbmc_project/Time/EpochTime/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>

#include <filesystem>

namespace phosphor
{
namespace dump
{

class Manager;

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

using OriginatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

/** @class BaseEntry
 *  @brief Abstract base class for Dump Entry.
 *  @details This class provides an interface for the
 *  xyz.openbmc_project.Dump.Entry DBus API and is intended
 *  to be subclassed by concrete implementations for different
 *  types of dumps.
 *
 *  The BaseEntry class is not directly instantiated. Instead,
 *  it is used as a common interface that all types of dump
 *  entries are expected to support. The methods declared in
 *  this class are pure virtual and must be implemented by
 *  any concrete derived class.
 *
 *  The BaseEntry class inherits from a series of interface classes,
 *  each providing a specific aspect of functionality.
 */
class BaseEntry : public EntryIfaces
{
  public:
    BaseEntry() = delete;
    BaseEntry(const BaseEntry&) = delete;
    BaseEntry& operator=(const BaseEntry&) = delete;
    BaseEntry(BaseEntry&&) = delete;
    BaseEntry& operator=(BaseEntry&&) = delete;
    ~BaseEntry() = default;

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
    BaseEntry(sdbusplus::bus_t& bus, const std::string& objPath,
              uint32_t dumpId, uint64_t timeStamp, uint64_t dumpSize,
              const std::filesystem::path& file, OperationStatus dumpStatus,
              std::string originId, OriginatorTypes originType,
              Manager& parent) :
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
    virtual void delete_();

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump
     */
    virtual void initiateOffload(std::string uri) override
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

    /** @brief Method to update an existing dump entry, once the dump creation
     *  is completed this function will be used to update the entry which got
     *  created during the dump request.
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     */
    virtual void markComplete(uint64_t timeStamp, uint64_t fileSize,
                              const std::filesystem::path& filePath) = 0;

    /** @brief Updates an existing dump entry once the dump creation is
     * completed
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     */
    virtual void markCompleteWithSourceId(uint64_t timeStamp, uint64_t fileSize,
                                          uint32_t sourceId) = 0;
  protected:
    /** @brief This entry's parent */
    Manager& parent;

    /** @brief This entry's id */
    uint32_t id;

    /** @Dump file name */
    std::filesystem::path file;
};

} // namespace dump
} // namespace phosphor
