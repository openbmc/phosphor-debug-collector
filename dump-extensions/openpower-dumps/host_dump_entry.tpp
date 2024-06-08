#include "dump_utils.hpp"
#include "host_dump_entry.hpp"
#include "host_transport_exts.hpp"
#include "op_dump_consts.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace openpower::dump::host
{

using namespace phosphor::logging;
template <typename Derived>
Entry<Derived>::Entry(sdbusplus::bus_t& bus, const std::string& objPath,
                      uint32_t dumpId, uint64_t timeStamp, uint64_t dumpSize,
                      phosphor::dump::OperationStatus status,
                      std::string originatorId,
                      phosphor::dump::originatorTypes originatorType,
                      phosphor::dump::Manager& parent, uint32_t transportId) :
    phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                          std::string(), status, originatorId, originatorType,
                          parent),
    transportId(transportId)
{}

template <typename Derived>
void Entry<Derived>::initiateOffload(std::string uri)
{
    lg2::info("Dump offload request id: {ID} uri: {URI} source dumpid:"
              "{SOURCE_DUMP_ID}",
              "ID", id, "URI", uri, "SOURCE_DUMP_ID", getSourceDumpId());
    phosphor::dump::Entry::initiateOffload(uri);
    phosphor::dump::host::requestOffload(getSourceDumpId());
}

template <typename Derived>
void Entry<Derived>::delete_()
{
    auto srcDumpID = getSourceDumpId();
    auto dumpId = id;

    // Offload URI will be set during dump offload
    // Prevent delete when offload is in progress
    if ((!offloadUri().empty()) && (phosphor::dump::isHostRunning()))
    {
        lg2::error(
            "Dump offload is in progress id: {DUMP_ID} srcdumpid: {SRC_DUMP_ID}",
            "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID);
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    lg2::info("Dump delete id: {DUMP_ID} srcdumpid: {SRC_DUMP_ID}", "DUMP_ID",
              dumpId, "SRC_DUMP_ID", srcDumpID);

    // Remove host system dump when host is up by using source dump id
    // which is present in system dump entry dbus object as a property.
    if ((phosphor::dump::isHostRunning()) && (srcDumpID != INVALID_SOURCE_ID))
    {
        try
        {
            phosphor::dump::host::requestDelete(srcDumpID, transportId);
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Error deleting dump from host id: {DUMP_ID} host id: {SRC_DUMP_ID} error: {ERROR}",
                "DUMP_ID", dumpId, "SRC_DUMP_ID", srcDumpID, "ERROR", e);
            elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
        }
    }

    phosphor::dump::Entry::delete_();
}

template <typename Derived>
void Entry<Derived>::update(uint64_t timeStamp, uint64_t dumpSize,
                            const uint32_t sourceId)
{
    setSourceDumpId(sourceId);
    elapsed(timeStamp);
    size(dumpSize);
    // TODO: Handle dump failure case with
    // #bm-openbmc/2808
    status(OperationStatus::Completed);
    completedTime(timeStamp);
    setDumpRequestStatus(Derived::HostResponse::Success);
}

} // namespace openpower::dump::host
