#include "config.h"

#include "dump_manager_system.hpp"

#include "dump_types.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"
#include "system_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

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
using SystemDumpEntry =
    phosphor::dump::new_::Entry<phosphor::dump::new_::SystemDump,
                                openpower::dump::host::DumpEntryHelper>;
std::map<NotifyDumpType, phosphor::dump::DumpTypes> notifyDumpTypeToEnum = {
    {NotifyDumpType::System, phosphor::dump::DumpTypes::SYSTEM}};

phosphor::dump::BaseEntry*
    Manager::getInProgressEntry(uint32_t dumpId, uint64_t size, uint32_t token)
{
    SystemDumpEntry* upEntry = nullptr;
    for (auto& entry : entries)
    {
        SystemDumpEntry* sysEntry =
            dynamic_cast<SystemDumpEntry*>(entry.second.get());

        // If there's already a completed entry with the input source id and
        // size, ignore this notification
        if (sysEntry->sourceDumpId() == dumpId && sysEntry->size() == size &&
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
        if (sysEntry->sourceDumpId() == INVALID_SOURCE_ID && upEntry == nullptr)
        {
            upEntry = sysEntry;
        }
    }

    return upEntry;
}

bool Manager::isHostStateValid()
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

std::string
    Manager::createEntry(uint32_t dumpId, uint64_t size,
                         phosphor::dump::OperationStatus status,
                         const std::string& originatorId,
                         phosphor::dump::OriginatorTypes originatorType,
                         phosphor::dump::host::HostTransport& hostTransport)
{
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;
    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    try
    {
        entries.emplace(id, std::make_unique<SystemDumpEntry>(
                                bus, objPath.c_str(), id, timeStamp, size,
                                dumpId, status, originatorId, originatorType,
                                hostTransport, *this));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error(
            "Error in creating system dump entry, errormsg: {ERROR}, "
            "OBJECTPATH: {OBJECT_PATH}, ID: {ID}, TIMESTAMP: {TIMESTAMP}, "
            "SIZE: {SIZE}, SOURCEID: {SOURCE_ID}",
            "ERROR", e, "OBJECT_PATH", objPath, "ID", id, "TIMESTAMP",
            timeStamp, "SIZE", size, "SOURCE_ID", dumpId);
        elog<InternalFailure>();
        return std::string();
    }
    lastEntryId++;
    return objPath.string();
}

void Manager::notifyDump(uint32_t dumpId, uint64_t size, NotifyDumpType type,
                         uint32_t token)
{
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;

    if (notifyDumpTypeToEnum.find(type) == notifyDumpTypeToEnum.end())
    {
        lg2::error("An invalid input passed for dump type");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("DUMP_TYPE"),
                              Argument::ARGUMENT_VALUE("INVALID INPUT"));
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
    SystemDumpEntry* upEntry =
        dynamic_cast<SystemDumpEntry*>(getInProgressEntry(dumpId, size, token));

    if (upEntry != nullptr)
    {
        lg2::info(
            "System Dump Notify: Updating dumpId:{ID} Source Id:{SOURCE_ID} "
            "Size:{SIZE}",
            "ID", upEntry->getDumpId(), "SOURCE_ID", dumpId, "SIZE", size);
        upEntry->markCompleteWithSourceId(timeStamp, size, dumpId);
        return;
    }

    lg2::info("System Dump Notify: creating new dump "
              "entry Source Id:{SOURCE_ID} Size:{SIZE}",
              "SOURCE_ID", dumpId, "SIZE", size);
    createEntry(dumpId, size, phosphor::dump::OperationStatus::Completed,
                std::string(), phosphor::dump::OriginatorTypes::Internal,
                hostTransport);
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

    if (!isHostStateValid())
    {
        lg2::error("System dump can be initiated only when the host is up "
                   "or quiesced or starting to poweroff");
        elog<NotAllowed>(
            Reason("System dump can be initiated only when the host is up "
                   "or quiesced or starting to poweroff"));
    }

    // Get the originator id and type from params
    std::string originatorId;
    phosphor::dump::OriginatorTypes originatorType;

    phosphor::dump::extractOriginatorProperties(params, originatorId,
                                                originatorType);

    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append(DIAG_MOD_TARGET); // unit to activate
    method.append("replace");
    bus.call_noreply(method);

    return createEntry(INVALID_SOURCE_ID, 0,
                       phosphor::dump::OperationStatus::InProgress,
                       originatorId, originatorType, hostTransport);
}

} // namespace system
} // namespace dump
} // namespace openpower
