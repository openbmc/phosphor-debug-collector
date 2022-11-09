#pragma once

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
     *  @param[in] originId - Id of the originator of the dump
     *  @param[in] originType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize, OperationStatus dumpStatus,
          std::string originId, originatorTypes originType, Manager& parent) :
        EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::emit_no_signals),
        parent(parent), id(dumpId)
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

  protected:
    /** @brief This entry's parent */
    Manager& parent;

    /** @brief This entry's id */
    uint32_t id;
};

} // namespace dump
} // namespace phosphor
