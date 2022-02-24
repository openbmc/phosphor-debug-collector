#include "resource_dump_entry.hpp"

#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "resource_dump_serialize.hpp"

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

void Entry::update(uint64_t timeStamp, uint64_t dumpSize, uint32_t sourceId)
{
    sourceDumpId(sourceId);
    elapsed(timeStamp);
    size(dumpSize);
    // TODO: Handled dump failure case with
    // #bm-openbmc/2808
    status(OperationStatus::Completed);
    completedTime(timeStamp);

    // serialize as dump is successfully completed
    serialize(*this);
}

void Entry::delete_()
{
    auto srcDumpID = sourceDumpId();
    auto dumpId = id;
    auto path = std::filesystem::path(RESOURCE_DUMP_SERIAL_PATH) /
                std::to_string(dumpId);

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
    try
    {
        std::filesystem::remove_all(path);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Log Error message and continue
        log<level::ERR>(
            fmt::format("Failed to delete dump file({}), errormsg({})",
                        path.string().c_str(), e.what())
                .c_str());
    }

    // Remove resource dump when host is up by using source dump id
    // which is present in resource dump entry dbus object as a property.
    if (phosphor::dump::isHostRunning())
    {
        phosphor::dump::host::requestDelete(srcDumpID,
                                            TRANSPORT_DUMP_TYPE_IDENTIFIER);
    }
}
} // namespace resource
} // namespace dump
} // namespace openpower
