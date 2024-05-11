#include "dump_entry_factory.hpp"

#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"

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

std::unique_ptr<phosphor::dump::Entry> DumpEntryFactory::createEntry(
    uint32_t id, phosphor::dump::DumpCreateParams& params)
{
    DumpParameters dumpParams = util::extractDumpParameters(params);

    id |= getDumpIdPrefix(dumpParams.type);
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

        default:
            util::throwInvalidArgument("DUMP_TYPE_NOT_VALID", "INVALID_INPUT");
    }
    return nullptr;
}

} // namespace openpower::dump
