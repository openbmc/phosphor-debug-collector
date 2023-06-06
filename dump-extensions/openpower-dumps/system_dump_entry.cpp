#include "system_dump_entry.hpp"

#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "op_dump_consts.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower
{
namespace dump
{
namespace system
{
// TODO #ibm-openbmc/issues/2859
// Revisit host transport impelementation
// This value is used to identify the dump in the transport layer to host,
constexpr auto TRANSPORT_DUMP_TYPE_IDENTIFIER = 3;
using namespace phosphor::logging;

void Entry::initiateOffload(std::string uri)
{
    lg2::info("System dump offload request id: {ID} uri: {URI} "
              "source dumpid: {SOURCE_DUMP_ID}",
              "ID", id, "URI", uri, "SOURCE_DUMP_ID", sourceDumpId());
    phosphor::dump::Entry::initiateOffload(uri);
    phosphor::dump::host::requestOffload(sourceDumpId());
}

void Entry::delete_()
{
    auto srcDumpID = sourceDumpId();
    auto dumpId = id;

    if ((!offloadUri().empty()) && (phosphor::dump::isHostRunning()))
    {
        lg2::error("Dump offload is in progress id: {DUMP_ID} "
                   "srcdumpid: {SRC_DUMP_ID}",
                   "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID);
        elog<sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed>(
            xyz::openbmc_project::Common::NotAllowed::REASON(
                "Dump offload is in progress"));
    }

    lg2::info("System dump delete id: {DUMP_ID} srcdumpid: {SRC_DUMP_ID}",
              "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID);

    // Remove host system dump when host is up by using source dump id
    // which is present in system dump entry dbus object as a property.
    if ((phosphor::dump::isHostRunning()) && (srcDumpID != INVALID_SOURCE_ID))
    {
        try
        {
            phosphor::dump::host::requestDelete(srcDumpID,
                                                TRANSPORT_DUMP_TYPE_IDENTIFIER);
        }
        catch (const std::exception& e)
        {
            lg2::error("Error deleting dump from host id: {DUMP_ID} "
                       "host id: {SRC_DUMP_ID} error: {ERROR}",
                       "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID, "ERROR", e);
            elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
        }
    }

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
}
} // namespace system
} // namespace dump
} // namespace openpower
