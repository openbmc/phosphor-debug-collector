#include "config.h"

#include "dump_manager_system.hpp"

#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"
#include "system_dump_entry.hpp"
#include "system_dump_entry_helper.hpp"
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

void Manager::notifyDump(uint32_t dumpId, uint64_t size, ComDumpType /* type */,
                     uint32_t token)
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
    using DumpEntryType = phosphor::dump::Entry<
        sdbusplus::xyz::openbmc_project::Dump::Entry::server::System,
        openpower::dump::DumpEntryHelper>;

    DumpEntryType* upEntry = getInProgressEntry<DumpEntryType>(dumpId, size,
                                                               token);
    if (upEntry != nullptr)
    {
        lg2::info(
            "System Dump Notify: Updating dumpId:{ID} Source Id:{SOURCE_ID} "
            "Size:{SIZE} ",
            "ID", upEntry->getDumpId(), "SOURCE_ID", dumpId, "SIZE", size);
        dynamic_cast<phosphor::dump::Entry<
            sdbusplus::xyz::openbmc_project::Dump::Entry::server::System,
            openpower::dump::DumpEntryHelper>*>(upEntry)
            ->markComplete(timeStamp, size, dumpId);
        return;
    }
    createEntry<DumpEntryType>(dumpId, size,
                               phosphor::dump::OperationStatus::Completed,
                               std::string(), phosphor::dump::OriginatorTypes::Internal);
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

    if (!isDumpAllowed())
    {
        lg2::error("System dump is unavailable now");
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

    using DumpEntryType = phosphor::dump::Entry<
        sdbusplus::xyz::openbmc_project::Dump::Entry::server::System,
        openpower::dump::DumpEntryHelper>;
    return createEntry<DumpEntryType>(INVALID_SOURCE_ID, 0,
                               phosphor::dump::OperationStatus::InProgress,
                               originatorId, originatorType);
}

} // namespace system
} // namespace dump
} // namespace openpower
