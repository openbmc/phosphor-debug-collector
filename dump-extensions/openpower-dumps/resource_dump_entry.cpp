#include "resource_dump_entry.hpp"

#include "dump_utils.hpp"
#include "host_transport_exts.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower
{
namespace dump
{
namespace resource
{
// TODO #ibm-openbmc/issues/2859
// Revisit host transport impelementation
// This value is used to identify the dump in the transport layer to host,
constexpr auto TRANSPORT_DUMP_TYPE_IDENTIFIER = 9;
using namespace phosphor::logging;

void Entry::initiateOffload(std::string uri)
{
    using NotAllowed =
        sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
    using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

    if (!phosphor::dump::isHostRunning())
    {
        elog<NotAllowed>(
            Reason("This dump can be offloaded only when the host is up"));
        return;
    }
    phosphor::dump::Entry::initiateOffload(uri);
    phosphor::dump::host::requestOffload(sourceDumpId());
}

void Entry::delete_()
{
    auto srcDumpID = sourceDumpId();

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();

    // Remove resource dump when host is up by using source dump id
    // which is present in resource dump entry dbus object as a property.
    if (phosphor::dump::isHostRunning())
    {
        phosphor::dump::host::requestDelete(srcDumpID,
                                            TRANSPORT_DUMP_TYPE_IDENTIFIER);
    }

    // Log PEL for dump delete
    log<level::INFO>("Log PEL for dump delete or offload");

    std::map<std::string, std::string> additionalData;
    additionalData.emplace("Dump ID", std::to_string(id));
    additionalData.emplace("Filename", uri);
    additionalData.emplace("Dump type", "Resource dump");
    constexpr auto severity =
        "xyz.openbmc_project.Logging.Entry.Level.Informational";
    createPEL(additionalData, severity,
              "xyz.openbmc_project.Dump.Error.Invalidate");
}
} // namespace resource
} // namespace dump
} // namespace openpower
