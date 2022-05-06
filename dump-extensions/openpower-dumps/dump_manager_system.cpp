#include "config.h"

#include "dump_manager_system.hpp"

#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"
#include "system_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

namespace openpower
{
namespace dump
{
namespace system
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

void Manager::notify(uint32_t dumpId, uint64_t size)
{
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
                    "System dump entry with source dump id:{SOURCE_ID} is "
                    "already present with entry id:{ID}",
                    "SOURCE_D", dumpId, "ID", sysEntry->getDumpId());
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
                    originatorTypes::Internal, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(
            fmt::format(
                "Error in creating system dump entry, errormsg({}), "
                "OBJECTPATH({}), ID({}), TIMESTAMP({}),SIZE({}), SOURCEID({})",
                e.what(), objPath.c_str(), id, timeStamp, size, dumpId)
                .c_str());
        report<InternalFailure>();
        return;
    }
    lastEntryId++;
    return;
}

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
    constexpr auto DIAG_MOD_TARGET = "obmc-host-crash@0.target";

    if (params.size() > CREATE_DUMP_MAX_PARAMS)
    {
        log<level::WARNING>(
            "System dump accepts not more than 2 additional parameters");
    }

    if (openpower::dump::util::isSystemDumpInProgress())
    {
        lg2::error("Another dump in progress or available to offload");
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    using NotAllowed =
        sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
    using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

    auto isHostRunning = false;
    Hphosphor::dump::ostState hostState;
    try
    {
        isHostRunning = phosphor::dump::isHostRunning();
        hostState = phosphor::dump::getHostState();
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "System state cannot be determined, system dump is not allowed: "
            "{EXCEPTION}",
            "EXCEPTION", e.what());
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

    // Get the originator id and type from params
    std::string originatorId;
    originatorTypes originatorType;

    phosphor::dump::extractOriginatorProperties(params, originatorId,
                                                originatorType);

    auto b = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append(DIAG_MOD_TARGET); // unit to activate
    method.append("replace");
    bus.call_noreply(method);

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
        log<level::ERR>(
            fmt::format("Error in creating system dump entry, errormsg({}), "
                        "OBJECTPATH({}), ID({})",
                        e.what(), objPath.c_str(), id)
                .c_str());
        elog<InternalFailure>();
        return std::string();
    }
    lastEntryId++;
    return objPath.string();
}

} // namespace system
} // namespace dump
} // namespace openpower
