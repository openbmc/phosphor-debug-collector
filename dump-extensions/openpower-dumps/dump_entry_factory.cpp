#include "dump_entry_factory.hpp"

#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"
#include "resource_dump_entry.hpp"

#include <com/ibm/Dump/Create/common.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace openpower::dump
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

std::unique_ptr<phosphor::dump::Entry> DumpEntryFactory::createSystemDumpEntry(
    uint32_t id, std::filesystem::path& objPath, uint64_t timeStamp,
    const DumpParameters& dumpParams)
{
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
        elog<NotAllowed>(Reason("System dump not allowed currently."));
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
        elog<NotAllowed>(Reason(
            "System dump can be initiated only when the host is up "
            "or quiesced or starting to poweroff"));
    }

    return std::make_unique<system::Entry>(
        bus, objPath.c_str(), id, timeStamp, 0, INVALID_SOURCE_ID,
        phosphor::dump::OperationStatus::InProgress, dumpParams.originatorId,
        dumpParams.originatorType, mgr);
}

std::unique_ptr<phosphor::dump::Entry>
    DumpEntryFactory::createResourceDumpEntry(
        uint32_t id, std::filesystem::path& objPath, uint64_t timeStamp,
        bool createSysDump, const DumpParameters& dumpParams)
{
    using NotAllowed =
        sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
    using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;
    if (!phosphor::dump::isHostRunning())
    {
        elog<NotAllowed>(
            Reason("Resource dump can be initiated only when the host is up"));
    }

    if (!dumpParams.userChallenge.has_value())
    {
        lg2::error("Required parameter user challenge is not provided");
        util::throwInvalidArgument("USER_CHALLENGE", "ARGUMENT_MISSING");
    }

    std::string vspString =
        (!dumpParams.vspString.has_value()) ? "" : *dumpParams.vspString;

    std::string userChallengeString = (!dumpParams.userChallenge.has_value())
                                          ? ""
                                          : *dumpParams.userChallenge;
    if (createSysDump)
    {
        return std::make_unique<system::Entry>(
            bus, objPath.c_str(), id, timeStamp, 0, INVALID_SOURCE_ID,
            phosphor::dump::OperationStatus::InProgress,
            dumpParams.originatorId, dumpParams.originatorType,
            system::SystemImpact::NonDisruptive, userChallengeString, mgr);
    }

    return std::make_unique<resource::Entry>(
        bus, objPath.c_str(), id, timeStamp, 0, INVALID_SOURCE_ID, vspString,
        userChallengeString, phosphor::dump::OperationStatus::InProgress,
        dumpParams.originatorId, dumpParams.originatorType, mgr);
}

std::unique_ptr<phosphor::dump::Entry> DumpEntryFactory::createEntry(
    uint32_t id, phosphor::dump::DumpCreateParams& params)
{
    DumpParameters dumpParams = util::extractDumpParameters(params);

    // If requested dump type is system and the vsp string is empty or "system"
    // the host will be creating a non-disruptive system dump and responding
    // with system dump type to avoid a never finished resource dump entry,
    // creating system dump entry in such cases

    bool createSystemDump =
        (dumpParams.type == OpDumpTypes::Resource &&
         (!dumpParams.vspString.has_value() || dumpParams.vspString->empty() ||
          toUpper(*dumpParams.vspString) == "SYSTEM"));

    uint32_t dumpIdPrefix = getDumpIdPrefix(
        createSystemDump ? OpDumpTypes::System : dumpParams.type);

    id |= dumpIdPrefix;
    std::string idStr = std::format("{:08X}", id);

    auto objPath = std::filesystem::path(baseEntryPath) / idStr;

    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    switch (dumpParams.type)
    {
        case OpDumpTypes::System:
            return createSystemDumpEntry(id, objPath, timeStamp, dumpParams);
        case OpDumpTypes::Resource:
            return createResourceDumpEntry(id, objPath, timeStamp,
                                           createSystemDump, dumpParams);
        case OpDumpTypes::Hostboot:
        case OpDumpTypes::SBE:
        case OpDumpTypes::Hardware:
        default:
            util::throwInvalidArgument("DUMP_TYPE_NOT_VALID", "INVALID_INPUT");
    }
    return nullptr;
}

std::optional<std::unique_ptr<phosphor::dump::Entry>>
    DumpEntryFactory::createOrUpdateHostEntry(
        OpDumpTypes type, uint64_t sourceDumpId, uint64_t size, uint32_t id,
        const std::map<uint32_t, std::unique_ptr<phosphor::dump::Entry>>&
            entries)
{
    switch (type)
    {
        case OpDumpTypes::System:
            return createOrUpdate<system::Entry>(type, sourceDumpId, size, id,
                                                 entries);
        case OpDumpTypes::Resource:
            return createOrUpdate<resource::Entry>(type, sourceDumpId, size, id,
                                                   entries);
        default:
            return std::nullopt;
    }
}

template <typename T>
std::optional<std::unique_ptr<phosphor::dump::Entry>>
    DumpEntryFactory::createOrUpdate(
        OpDumpTypes dumpType, uint64_t srcDumpId, uint64_t size, uint32_t id,
        const std::map<uint32_t, std::unique_ptr<phosphor::dump::Entry>>&
            entries)
{
    static_assert(std::is_base_of<phosphor::dump::Entry, T>::value,
                  "T must be derived from phosphor::dump::Entry");

    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // If there is an entry with invalid id update that.
    // If there a completed one with same source id ignore it
    // if there is no invalid id, create new entry
    T* upEntry = nullptr;

    for (const auto& entry : entries)
    {
        if (getDumpTypeFromId(entry.second->getDumpId()) != dumpType)
        {
            continue;
        }
        auto dumpEntry = dynamic_cast<T*>(entry.second.get());
        // If there is already a completed entry with input source id then
        // ignore this notification.
        if ((dumpEntry->sourceDumpId() == srcDumpId) &&
            (dumpEntry->status() == phosphor::dump::OperationStatus::Completed))
        {
            lg2::info("Dump entry with source dump id: {DUMP_ID} is already "
                      "present with entry id: {ENTRY_ID}",
                      "DUMP_ID", std::format("{:08X}", srcDumpId), "ENTRY_ID",
                      std::format("{:08X}", dumpEntry->getDumpId()));
            return std::nullopt;
        }

        // When searching for the dump to update, find the first entry with
        // INVALID_SOURCE_ID and remember it. However, continue searching
        // through all the entries to ensure that the incoming dump has not
        // already been notified by checking for the same source ID.
        if ((dumpEntry->status() ==
             phosphor::dump::OperationStatus::InProgress) &&
            (dumpEntry->sourceDumpId() == INVALID_SOURCE_ID) &&
            (upEntry == nullptr))
        {
            upEntry = dumpEntry;
        }
    }

    if (upEntry != nullptr)
    {
        lg2::info("Dump Notify: Updating dumpId: {DUMP_ID} with "
                  "source Id: {SOURCE_ID} Size: {SIZE}",
                  "DUMP_ID", std::format("{:08X}", upEntry->getDumpId()),
                  "SOURCE_ID", std::format("{:08X}", srcDumpId), "SIZE", size);
        upEntry->update(timeStamp, size, srcDumpId);
        return std::nullopt;
    }

    id |= getDumpIdPrefix(dumpType);
    std::string idStr = std::format("{:08X}", id);
    auto objPath = std::filesystem::path(baseEntryPath) / idStr;

    // TODO: Get the originator Id, type from the persisted file.
    // For now replacing it with null

    try
    {
        lg2::info("Dump Notify: creating new dump entry dumpId: {DUMP_ID} "
                  "Id: {ID} Size: {SIZE}",
                  "DUMP_ID", idStr, "ID", srcDumpId, "SIZE", size);

        return std::make_unique<T>(
            bus, objPath.c_str(), id, timeStamp, size, srcDumpId,
            phosphor::dump::OperationStatus::Completed, std::string(),
            phosphor::dump::originatorTypes::Internal, mgr);
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error(
            "Error in creating resource dump entry, errormsg: {ERROR}, "
            "OBJECTPATH: {OBJECT_PATH}, ID: {ID}, TIMESTAMP: {TIMESTAMP}, "
            "SIZE: {SIZE}, SOURCEID: {SOURCE_ID}",
            "ERROR", e, "OBJECT_PATH", objPath, "ID", idStr, "TIMESTAMP",
            timeStamp, "SIZE", size, "SOURCE_ID", srcDumpId);
        report<InternalFailure>();
        return std::nullopt;
    }
}

} // namespace openpower::dump
