#include "config.h"

#include "dump_manager_openpower.hpp"

#include "dump_entry_factory.hpp"
#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"
#include "system_dump_entry.hpp"

#include <com/ibm/Dump/Create/common.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower::dump
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

void Manager::notifyDump(uint32_t sourceDumpId, uint64_t size,
                         NotifyDumpTypes type, [[maybe_unused]] uint32_t token)
{
    DumpEntryFactory dumpFact(bus, baseEntryPath, *this);

    auto optEntry = dumpFact.createOrUpdateHostEntry(
        convertNotifyToCreateType(type), sourceDumpId, size, lastEntryId + 1,
        entries);
    if (optEntry)
    {
        auto& entry = *optEntry;
        entries.insert(std::make_pair(entry->getDumpId(), std::move(entry)));
        lastEntryId++;
    }
}

sdbusplus::message::object_path Manager::createDump(
    phosphor::dump::DumpCreateParams params)
{
    try
    {
        DumpEntryFactory dumpFact(bus, baseEntryPath, *this);

        auto dumpEntry = dumpFact.createEntry(lastEntryId + 1, params);
        if (!dumpEntry)
        {
            lg2::error("Dump entry creation failed");
            return {};
        }

        uint32_t id = dumpEntry->getDumpId();
        entries.insert(std::make_pair(id, std::move(dumpEntry)));
        std::string idStr = std::format("{:08X}", id);
        lastEntryId++;
        return baseEntryPath + "/" + idStr;
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to create dump: {ERROR}", "ERROR", e);
        throw;
    }
    return {};
}

} // namespace openpower::dump
