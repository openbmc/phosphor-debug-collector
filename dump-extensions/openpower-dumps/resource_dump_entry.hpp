#pragma once

#include "com/ibm/Dump/Entry/Resource/server.hpp"
#include "dump_entry.hpp"
#include "op_dump_consts.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <chrono>

namespace openpower
{
namespace dump
{
namespace resource
{
template <typename T>
using ServerObject = typename sdbusplus::server::object_t<T>;

using EntryIfaces = sdbusplus::server::object_t<
    sdbusplus::com::ibm::Dump::Entry::server::Resource>;

using originatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

class Manager;

/** @class Entry
 *  @brief Resource Dump Entry implementation.
 *  @details An extension to Dump::Entry class and
 *  A concrete implementation for the
 *  com::ibm::Dump::Entry::Resource DBus API
 */
class Entry : virtual public phosphor::dump::Entry, virtual public EntryIfaces
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    ~Entry() = default;

    /** @brief Constructor for the resource dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] dumpId - Dump id.
     *  @param[in] timeStamp - Dump creation timestamp
     *             since the epoch.
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     *  @param[in] vspStr- Input to host to generate the resource dump.
     *  @param[in] usrChallenge - User Challenge needed by host to validate the
     *             request.
     *  @param[in] status - status  of the dump.
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId,
          std::string vspStr, std::string usrChallenge,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, phosphor::dump::Manager& parent) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                              std::string(), status, originatorId,
                              originatorType, parent),
        EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit)
    {
        sourceDumpId(sourceId);
        vspString(vspStr);
        userChallenge(usrChallenge);
        // Emit deferred signal.
        this->openpower::dump::resource::EntryIfaces::emit_object_added();
    };

    /** @brief Constructor for the resource dump Entry Object for system
     * generated dumps, in such cases vsp string and user challenge wont be
     * available.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] dumpId - Dump id.
     *  @param[in] timeStamp - Dump creation timestamp
     *             since the epoch.
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     *  @param[in] status - status  of the dump.
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     *  @param[in] parent - The dump entry's parent.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, phosphor::dump::Manager& parent) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                              std::string(), status, originatorId,
                              originatorType, parent),
        EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit)
    {
        using HostResponse =
            sdbusplus::common::com::ibm::dump::entry::Resource::HostResponse;
        sourceDumpId(sourceId);
        vspString("");
        userChallenge("");
        dumpRequestStatus(HostResponse::Success);

        // Emit deferred signal.
        this->openpower::dump::resource::EntryIfaces::emit_object_added();
        serializeEntry();
    };

    /** @brief Constructor for creating a Resource dump entry with default
     * values
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to.
     *  @param[in] dumpId - Unique identifier for the dump.
     *  @param[in] parent - Reference to the managing dump manager.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          phosphor::dump::Manager& parent) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, 0, 0, "",
                              phosphor::dump::OperationStatus::InProgress, "",
                              originatorTypes::Internal, parent),
        EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit)
    {}

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump.
     */
    void initiateOffload(std::string uri) override;

    /** @brief Method to update an existing dump entry
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - The id of dump in the origin.
     */
    void update(uint64_t timeStamp, uint64_t dumpSize, uint32_t sourceId)
    {
        using HostResponse =
            sdbusplus::common::com::ibm::dump::entry::Resource::HostResponse;
        sourceDumpId(sourceId);
        elapsed(timeStamp);
        size(dumpSize);
        // TODO: Handled dump failure case with
        // #bm-openbmc/2808
        status(OperationStatus::Completed);
        completedTime(timeStamp);
        dumpRequestStatus(HostResponse::Success);

        serializeEntry();
    }

    /**
     * @brief Delete resource dump in host memory and the entry dbus object
     */
    void delete_() override;

    /** @brief Serialize the resource dump entry
     *  @param[in] filePath - Path to the file where the entry will be
     * serialized
     */
    void serialize(const std::filesystem::path& filePath) override;

    /** @brief Deserialize the resource dump entry
     *  @param[in] filePath - Path to the file from where the entry will be
     * deserialized
     */
    void deserialize(const std::filesystem::path& filePath) override;

    /**
     * @brief Make serialize path and serialize the entry.
     */
    inline void serializeEntry()
    {
        std::string idStr = std::format("{:08X}", getDumpId());
        const std::filesystem::path serializedFilePath =
            std::filesystem::path(openpower::dump::OP_DUMP_PATH) / idStr /
            ".preserve" / "serialized_entry.bin";
        serialize(serializedFilePath);
    }

    /**
     * @brief Remove the folder containing serialized entry
     */
    inline void removeSerializedEntry()
    {
        std::string idStr = std::format("{:08X}", getDumpId());
        const std::filesystem::path serializedDir =
            std::filesystem::path(openpower::dump::OP_DUMP_PATH) / idStr;
        try
        {
            std::filesystem::remove_all(serializedDir);
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            // Log Error message and continue
            lg2::error(
                "Failed to delete directory, path: {PATH} errormsg: {ERROR}",
                "PATH", serializedDir, "ERROR", e);
        }
    }
};

} // namespace resource
} // namespace dump
} // namespace openpower
