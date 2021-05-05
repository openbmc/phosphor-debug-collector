#include "system_dump_entry_helper.hpp"

#include "base_dump_entry.hpp"
#include "com/ibm/Dump/Entry/Resource/server.hpp"
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "op_dump_util.hpp"
#include "xyz/openbmc_project/Dump/Entry/System/server.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower
{
namespace dump
{
constexpr auto INVALID_SOURCE_ID = 0xFFFFFF;

void DumpEntryHelper::initiateOffload(uint32_t id, uint32_t sourceDumpId,
                                      const std::string& uri)
{
    lg2::info(
        "Dump offload request id: {DUMP_ID} uri: {URI} source dumpid{SOURCE_ID}",
        "DUMP_ID", id, "URI", uri, "SOURCE_ID", sourceDumpId);
    dumpEntry.phosphor::dump::BaseEntry::initiateOffload(uri);
    phosphor::dump::host::requestOffload(sourceDumpId);
}

void DumpEntryHelper::delete_(uint32_t dumpId, uint32_t sourceDumpId,
                              uint8_t transportId,
                              const std::string& dumpPathOffLoadUri)
{
    auto hostRunning = phosphor::dump::isHostRunning();
    if (!dumpPathOffLoadUri.empty() && hostRunning)
    {
        lg2::error(
            "Dump offload in progress id: {ID} srcdumpid: {SOURCE_DUMP_ID}",
            "ID", dumpId, "SOURCE_DUMP_ID", sourceDumpId);
        phosphor::logging::elog<
            sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    lg2::info("System dump delete id: {ID} srcdumpid: {SOURCE_DUMP_ID}", "ID",
              dumpId, "SOURCE_DUMP_ID", sourceDumpId);

    if (hostRunning && sourceDumpId != INVALID_SOURCE_ID)
    {
        try
        {
            phosphor::dump::host::requestDelete(sourceDumpId, transportId);
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Error deleting dump from host id: {ID} srcdumpid: {SOURCE_DUMP_ID} error: {ERROR_MSG}",
                "ID", dumpId, "SOURCE_DUMP_ID", sourceDumpId, "ERROR_MSG",
                e.what());
            phosphor::logging::elog<
                sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
        }
    }
}

sdbusplus::message::unix_fd DumpEntryHelper::getFileHandle()
{
    phosphor::logging::elog<
        sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    return 0;
}

} // namespace dump
} // namespace openpower
