#pragma once

#include "com/ibm/Dump/Notify/server.hpp"
#include "dump_manager.hpp"
#include "dump_utils.hpp"
#include "op_dump_util.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

namespace openpower
{
namespace dump
{
namespace system
{

constexpr uint32_t INVALID_SOURCE_ID = 0xFFFFFFFF;
using NotifyIface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::com::ibm::Dump::server::Notify>;
using ComDumpType = sdbusplus::common::com::ibm::dump::Notify::DumpType;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

    template <typename T>
    concept DumpEntryConcept = requires(T a) {
                            {
                                a.sourceDumpId()
                            } -> std::convertible_to<uint32_t>;
                            {
                                a.size()
                            } -> std::convertible_to<uint64_t>;
                            {
                                a.status()
                            } -> std::same_as<phosphor::dump::OperationStatus>;
                            {
                                a.delete_()
                            } -> std::same_as<void>;
                        };

/** @class Manager
 *  @brief System Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Notify DBus API
 */
class Manager :
    virtual public NotifyIface,
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
     */
    Manager(sdbusplus::bus_t& bus, const char* path,
            const std::string& baseEntryPath) :
        NotifyIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath)
    {}

    void restore() override
    {
        // TODO #2597  Implement the restore to restore the dump entries
        // after the service restart.
    }

    /** @brief Notify the system dump manager about creation of a new dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notifyDump(uint32_t dumpId, uint64_t size, ComDumpType type,
                uint32_t token = 0) override;

    /** @brief Implementation for CreateDump
     *  Method to create a new system dump entry when user
     *  requests for a new system dump.
     *
     *  @return object_path - The path to the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

    template <DumpEntryConcept DumpEntryType>
    DumpEntryType* getInProgressEntry(uint32_t dumpId, uint64_t size,
                                   uint32_t token)
    {
        DumpEntryType* upEntry = nullptr;

        for (auto& entry : entries)
        {
            DumpEntryType* sysEntry =
                dynamic_cast<DumpEntryType*>(entry.second.get());

            // If there's already a completed entry with the input source id and
            // size, ignore this notification
            if (sysEntry->sourceDumpId() == dumpId &&
                sysEntry->size() == size &&
                (token == 0 || sysEntry->token() == token))
            {
                if (sysEntry->status() ==
                    phosphor::dump::OperationStatus::Completed)
                {
                    lg2::error("A completed entry with same details found "
                               "probably due to a duplicate notification"
                               "dump id: {SOURCE_ID} entry id: {ID}",
                               "SOURCE_D", dumpId, "ID", upEntry->getDumpId());
                    return nullptr;
                }
                else
                {
                    return sysEntry;
                }
            }

            // If the dump id is the same but the size is different, then this
            // is a new dump. So, delete the stale entry.
            if (sysEntry->sourceDumpId() == dumpId)
            {
                sysEntry->delete_();
            }

            // Save the first entry with INVALID_SOURCE_ID, but don't return it
            // until we've checked all entries.
            if (sysEntry->sourceDumpId() == INVALID_SOURCE_ID &&
                upEntry == nullptr)
            {
                upEntry = sysEntry;
            }
        }

        return upEntry;
    }

    template <typename DumpEntryType>
    std::string createEntry(uint32_t dumpId, uint64_t size,
                              phosphor::dump::OperationStatus status,
                              const std::string& originatorId,
                              phosphor::dump::OriginatorTypes originatorType)
    {
        // Get the timestamp
        uint64_t timeStamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

        // Get the id
        auto id = currentEntryId() + 1;
        auto idString = std::to_string(id);
        auto objPath = std::filesystem::path(baseEntryPath) / idString;

        try
        {
            lg2::info("System Dump Notify: creating new dump "
                      "entry dumpId:{ID} Source Id:{SOURCE_ID} Size:{SIZE}",
                      "ID", id, "SOURCE_ID", dumpId, "SIZE", size);
            entries.emplace(id, std::make_unique<DumpEntryType>(
                                    bus, objPath.c_str(), id, timeStamp, size,
                                    dumpId, status, originatorId,
                                    originatorType, 3, *this));
        }
        catch (const std::invalid_argument& e)
        {
            using Unavailable =
            sdbusplus::xyz::openbmc_project::Common::Error::Unavailable;
            lg2::error(
                "Error in creating system dump entry, errormsg: {ERROR}, "
                "OBJECTPATH: {OBJECT_PATH}, ID: {ID}, TIMESTAMP: {TIMESTAMP}, "
                "SIZE: {SIZE}, SOURCEID: {SOURCE_ID}",
                "ERROR", e.what(), "OBJECT_PATH", objPath.string(), "ID", id,
                "TIMESTAMP", timeStamp, "SIZE", size, "SOURCE_ID", dumpId);
            elog<Unavailable>();
        }
        incrementLastEntryId();
        return objPath.string();
    }

    inline bool isDumpAllowed()
    {
        if (openpower::dump::util::isSystemDumpInProgress(bus))
        {
            lg2::error("Another dump in progress or available to offload");
            return false;
        }
        return true;
    }

    inline bool isHostStateValid()
    {
        using Unavailable =
            sdbusplus::xyz::openbmc_project::Common::Error::Unavailable;
        auto isHostRunning = false;
        phosphor::dump::HostState hostState;

        try
        {
            isHostRunning = phosphor::dump::isHostRunning();
            hostState = phosphor::dump::getHostState();
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "System state cannot be determined, system dump is not allowed: "
                "{ERROR}",
                "ERROR", e);
            elog<Unavailable>();
            return false;
        }

        bool isHostQuiesced = hostState == phosphor::dump::HostState::Quiesced;
        bool isHostTransitioningToOff =
            hostState == phosphor::dump::HostState::TransitioningToOff;

        if (!isHostRunning && !isHostQuiesced && !isHostTransitioningToOff)
        {
            return false;
        }

        return true;
    }
};

} // namespace system
} // namespace dump
} // namespace openpower
