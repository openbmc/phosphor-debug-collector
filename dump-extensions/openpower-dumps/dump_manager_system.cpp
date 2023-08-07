#include "config.h"

#include "dump_manager_system.hpp"

#include "base_dump_entry.hpp"
#include "dump_types.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"
#include "system_dump_entry.hpp"
#include "system_dump_helper.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

namespace openpower
{
namespace dump
{

constexpr auto SYSTEM_DUMP = "SYSTEM_DUMP";

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using SystemDumpEntry =
    phosphor::dump::new_::Entry<phosphor::dump::new_::SystemDump,
                                openpower::dump::host::DumpEntryHelper>;

std::map<NotifyDumpType, phosphor::dump::DumpTypes> notifyDumpTypeToEnum = {
    {NotifyDumpType::System, phosphor::dump::DumpTypes::SYSTEM}};

Manager::Manager(sdbusplus::bus_t& bus, const char* path,
                 const std::string& baseEntryPath,
                 phosphor::dump::host::HostTransport& hostTransport) :
    NotifyIface(bus, path),
    phosphor::dump::BaseManager(bus, path), hostTransport(hostTransport),
    baseEntryPath(baseEntryPath)
{
    helpers.emplace(
        phosphor::dump::DumpTypes::SYSTEM,
        std::make_unique<system::DumpHelper>(bus, *this, hostTransport));
}

phosphor::dump::BaseEntry*
    Manager::getInProgressEntry(phosphor::dump::DumpTypes type, uint32_t dumpId,
                                uint64_t size, uint32_t token)
{
    SystemDumpEntry* upEntry = nullptr;
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;

    DumpEntryList entries;

    try
    {
        entries = std::get<1>(dumpEntries.at(type));
    }
    catch (std::exception& e)
    {
        lg2::error("An invalid input passed for dump type error: {ERROR}",
                   "ERROR", e);
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("DUMP_TYPE"),
                              Argument::ARGUMENT_VALUE("INVALID INPUT"));
    }

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

void Manager::notifyDump(uint32_t dumpId, uint64_t size, NotifyDumpType type,
                         uint32_t token)
{
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;

    auto it = notifyDumpTypeToEnum.find(type);
    if (it == notifyDumpTypeToEnum.end())
    {
        lg2::error("An invalid input passed for dump type");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("DUMP_TYPE"),
                              Argument::ARGUMENT_VALUE("INVALID INPUT"));
    }
    phosphor::dump::DumpTypes dumpType = it->second;

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
    auto upEntry = getInProgressEntry(dumpType, dumpId, size, token);

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
    helpers[dumpType]->createEntry(dumpId, size, phosphor::dump::OperationStatus::Completed,
        std::string(), phosphor::dump::OriginatorTypes::Internal,
        hostTransport);
}

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    phosphor::dump::DumpTypes dumpType;
    std::string type = phosphor::dump::extractParameter<std::string>(
        convertCreateParametersToString(CreateParameters::DumpType), params);
    if (!type.empty())
    {
        dumpType = phosphor::dump::validateDumpType(type, SYSTEM_DUMP);
    }

    std::unique_ptr<phosphor::dump::BaseEntry> entry;
    auto it = helpers.find(dumpType);
    if (it != helpers.end())
    {
        // Call createDump on the appropriate helper
        return it->second->createDump(params);
    }
    else
    {
        // Handle
    }

    return std::string();
}

} // namespace dump
} // namespace openpower
