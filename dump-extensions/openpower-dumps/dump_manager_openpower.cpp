#include "config.h"

#include "dump_manager_openpower.hpp"

#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"
#include "system_dump_entry.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower::dump
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

void Manager::notifyDump(uint32_t dumpId, uint64_t size, NotifyDumpTypes type,
                         [[maybe_unused]] uint32_t token)
{
    // Proceed only if the type is system
    if (type != NotifyDumpTypes::System)
    {
        return;
    }
    // Get the timestamp
    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // A system dump can be created due to a fault in the server or by a user
    // request. A system dump by fault is first reported here, but for a
    // user-requested dump, an entry will be created first with an invalid
    // source id. Since only one system dump creation is allowed at a time, if
    // there's an entry with an invalid sourceId, we will update that entry.
    openpower::dump::system::Entry* upEntry = nullptr;
    for (auto& entry : entries)
    {
        openpower::dump::system::Entry* sysEntry =
            dynamic_cast<openpower::dump::system::Entry*>(entry.second.get());

        // If there's already a completed entry with the input source id and
        // size, ignore this notification
        if ((sysEntry->sourceDumpId() == dumpId) && (sysEntry->size() == size))
        {
            if (sysEntry->status() ==
                phosphor::dump::OperationStatus::Completed)
            {
                lg2::info(
                    "System dump entry with source dump id:{SOURCE_ID} and "
                    "size: {SIZE} is already present with entry id:{ID}",
                    "SOURCE_ID", dumpId, "SIZE", size, "ID",
                    sysEntry->getDumpId());
                return;
            }
            else
            {
                lg2::error("A duplicate notification for an incomplete dump "
                           "dump id: {SOURCE_ID} entry id: {ID}",
                           "SOURCE_D", dumpId, "ID", sysEntry->getDumpId());
                upEntry = sysEntry;
                break;
            }
        }
        else if (sysEntry->sourceDumpId() == dumpId)
        {
            // If the dump id is the same but the size is different, then this
            // is a new dump. So, delete the stale entry and prepare to create a
            // new one.
            lg2::info("A previous dump entry found with same source id: "
                      "{SOURCE_ID}, deleting it, entry id: {DUMP_ID}",
                      "SOURCE_ID", dumpId, "DUMP_ID", sysEntry->getDumpId());
            sysEntry->delete_();
            // No 'break' here, as we need to continue checking other entries.
        }

        // Save the first entry with INVALID_SOURCE_ID, but continue in the loop
        // to ensure the new entry is not a duplicate.
        if ((sysEntry->sourceDumpId() == INVALID_SOURCE_ID) &&
            (upEntry == nullptr))
        {
            upEntry = sysEntry;
        }
    }

    if (upEntry != nullptr)
    {
        lg2::info(
            "System Dump Notify: Updating dumpId:{ID} Source Id:{SOURCE_ID} "
            "Size:{SIZE}",
            "ID", upEntry->getDumpId(), "SOURCE_ID", dumpId, "SIZE", size);
        upEntry->update(timeStamp, size, dumpId);
        return;
    }

    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;

    // TODO: Get the originator Id, Type from the persisted file.
    // For now replacing it with null
    try
    {
        lg2::info("System Dump Notify: creating new dump "
                  "entry dumpId:{ID} Source Id:{SOURCE_ID} Size:{SIZE}",
                  "ID", id, "SOURCE_ID", dumpId, "SIZE", size);
        entries.insert(std::make_pair(
            id, std::make_unique<system::Entry>(
                    bus, objPath.c_str(), id, timeStamp, size, dumpId,
                    phosphor::dump::OperationStatus::Completed, std::string(),
                    phosphor::dump::originatorTypes::Internal, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error(
            "Error in creating system dump entry, errormsg: {ERROR}, "
            "OBJECTPATH: {OBJECT_PATH}, ID: {ID}, TIMESTAMP: {TIMESTAMP}, "
            "SIZE: {SIZE}, SOURCEID: {SOURCE_ID}",
            "ERROR", e, "OBJECT_PATH", objPath, "ID", id, "TIMESTAMP",
            timeStamp, "SIZE", size, "SOURCE_ID", dumpId);
        report<InternalFailure>();
        return;
    }
    lastEntryId++;
    return;
}

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    if (params.size() > CREATE_DUMP_MAX_PARAMS)
    {
        lg2::warning(
            "System dump accepts not more than 2 additional parameters");
    }
    using Unavailable =
        sdbusplus::xyz::openbmc_project::Common::Error::Unavailable;

    if (openpower::dump::util::isSystemDumpInProgress(bus))
    {
        lg2::error("Another dump in progress or available to offload");
        elog<Unavailable>();
    }

    using NotAllowed =
        sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
    using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

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
        return std::string();
    }
    bool isHostQuiesced = hostState == phosphor::dump::HostState::Quiesced;
    bool isHostTransitioningToOff =
        hostState == phosphor::dump::HostState::TransitioningToOff;
    // Allow creating system dump only when the host is up or quiesced
    // starting to power off
    if (!isHostRunning && !isHostQuiesced && !isHostTransitioningToOff)
    {
        lg2::error("System dump can be initiated only when the host is up "
                   "or quiesced or starting to poweroff");
        elog<NotAllowed>(
            Reason("System dump can be initiated only when the host is up "
                   "or quiesced or starting to poweroff"));
        return std::string();
    }

    std::string originatorId;
    phosphor::dump::originatorTypes originatorType;
    phosphor::dump::extractOriginatorProperties(params, originatorId,
                                                originatorType);

    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;
    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    try
    {
        entries.insert(std::make_pair(
            id, std::make_unique<system::Entry>(
                    bus, objPath.c_str(), id, timeStamp, 0, INVALID_SOURCE_ID,
                    phosphor::dump::OperationStatus::InProgress, originatorId,
                    originatorType, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error("Error in creating system dump entry, errormsg: {ERROR}, "
                   "OBJECTPATH: {OBJECT_PATH}, ID: {ID}",
                   "ERROR", e, "OBJECT_PATH", objPath, "ID", id);
        elog<InternalFailure>();
        return std::string();
    }
    lastEntryId++;
    return objPath.string();
}

} // namespace openpower::dump
