#include "system_dump_helper.hpp"

#include "base_dump_entry.hpp"
#include "dump_manager_system.hpp"
#include "dump_utils.hpp"
#include "op_dump_util.hpp"
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

std::string DumpHelper::createDump(phosphor::dump::DumpCreateParams& params)
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
    constexpr auto DIAG_MOD_TARGET = "obmc-host-crash@0.target";

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

bool DumpHelper::isHostStateValid()
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
    DumpHelper::createEntry(uint32_t dumpId, uint64_t size,
                            phosphor::dump::OperationStatus status,
                            const std::string& originatorId,
                            phosphor::dump::OriginatorTypes originatorType,
                            phosphor::dump::host::HostTransport& hostTransport)
{
    auto id = mgr.getNextId(dumpType);
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(mgr.getBaseEntryPath()) / idString;
    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    try
    {
        std::unique_ptr<SystemDumpEntry> entry =
            std::make_unique<SystemDumpEntry>(
                bus, objPath.c_str(), id, timeStamp, size, dumpId, status,
                originatorId, originatorType, hostTransport, mgr);
        mgr.insertToDumpEntries<SystemDumpEntry>(dumpType, std::move(entry));
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
    return objPath.string();
}

} // namespace system
} // namespace dump
} // namespace openpower
