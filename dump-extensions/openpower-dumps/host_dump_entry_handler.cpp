#include "base_dump_entry.hpp"
#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "system_dump_entry.hpp"

namespace openpower
{
namespace dump
{
namespace host
{
void DumpEntryHelper::delete_(uint32_t dumpId, uint32_t srcDumpId,
                              const std::string uri)
{
    using namespace phosphor::logging;
    // Offload URI will be set during dump offload
    // Prevent delete when offload is in progress
    if ((!uri.empty()) && (phosphor::dump::isHostRunning()))
    {
        lg2::error("Dump offload is in progress id: {DUMP_ID} "
                   "srcdumpid: {SRC_DUMP_ID}",
                   "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpId);
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    lg2::info("System dump delete id: {DUMP_ID} srcdumpid: {SRC_DUMP_ID}",
              "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpId);

    // Remove host system dump when host is up by using source dump id
    // which is present in system dump entry dbus object as a property.
    if ((phosphor::dump::isHostRunning()) && (srcDumpId != INVALID_SOURCE_ID))
    {
        try
        {
            hostTransport.requestDelete(srcDumpId, dumpType);
        }
        catch (const std::exception& e)
        {
            lg2::error("Error deleting dump from host id: {DUMP_ID} "
                       "host id: {SRC_DUMP_ID} error: {ERROR}",
                       "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpId, "ERROR", e);
            elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
        }
    }
}

void DumpEntryHelper::initiateOffload(uint32_t srcDumpId)
{
    hostTransport.requestOffload(srcDumpId);
}

sdbusplus::message::unix_fd DumpEntryHelper::getFileHandle(uint32_t)
{
    using namespace phosphor::logging;

    lg2::error("This function is unavailable on this type of dump.");
    elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    return 0;
}

} // namespace host
} // namespace dump
} // namespace openpower
