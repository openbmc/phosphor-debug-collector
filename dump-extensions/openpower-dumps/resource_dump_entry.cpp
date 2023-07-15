#include "resource_dump_entry.hpp"

#include "dump_manager_resource.hpp"
#include "dump_utils.hpp"
#include "op_dump_consts.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower
{
namespace dump
{
namespace resource
{

using namespace phosphor::logging;

void Entry::initiateOffload(std::string uri)
{
    using NotAllowed =
        sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
    using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;
    lg2::info("Resource dump offload request id: {ID} uri: {URI} "
              "source dumpid: {SOURCE_DUMP_ID}",
              "ID", id, "URI", uri, "SOURCE_DUMP_ID", sourceDumpId());

    if (!phosphor::dump::isHostRunning())
    {
        elog<NotAllowed>(
            Reason("This dump can be offloaded only when the host is up"));
        return;
    }
    phosphor::dump::Entry::initiateOffload(uri);
    dynamic_cast<Manager&>(parent).getHostTransport()->requestOffload(
        sourceDumpId());
}

void Entry::delete_()
{
    auto srcDumpID = sourceDumpId();
    auto dumpId = id;

    if ((!offloadUri().empty()) && (phosphor::dump::isHostRunning()))
    {
        lg2::error("Dump offload is in progress, cannot delete dump, "
                   "id: {DUMP_ID} srcdumpid: {SRC_DUMP_ID}",
                   "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID);
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    lg2::info("Resource dump delete id: {DUMP_ID} srcdumpid: {SRC_DUMP_ID}",
              "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID);

    // Remove resource dump when host is up by using source dump id

    // which is present in resource dump entry dbus object as a property.
    if ((phosphor::dump::isHostRunning()) && (srcDumpID != INVALID_SOURCE_ID))
    {
        try
        {
            dynamic_cast<Manager&>(parent).getHostTransport()->requestDelete(
                srcDumpID, phosphor::dump::DumpTypes::RESOURCE);
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
} // namespace resource
} // namespace dump
} // namespace openpower
