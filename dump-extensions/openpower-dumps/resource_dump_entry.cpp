#include "resource_dump_entry.hpp"

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
    phosphor::dump::host::requestOffload(sourceDumpId());
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

void Entry::serialize(const std::filesystem::path& filePath)
{
    try
    {
        std::filesystem::path dir = filePath.parent_path();
        if (!std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir);
        }

        std::ofstream ofs(filePath, std::ios::binary);
        if (!ofs.is_open())
        {
            lg2::error("Failed to open file for serialization: {PATH} ", "PATH",
                       filePath);
        }
        cereal::BinaryOutputArchive archive(ofs);
        archive(sourceDumpId(), size(), originatorId(), originatorType(),
                completedTime(), elapsed(), startTime());
    }
    catch (const std::exception& e)
    {
        lg2::error("Serialization error: {PATH} {ERROR} ", "PATH", filePath,
                   "ERROR", e);
    }
}

void Entry::deserialize(const std::filesystem::path& filePath)
{
    try
    {
        std::ifstream ifs(filePath, std::ios::binary);
        if (!ifs.is_open())
        {
            lg2::error("Failed to open file for deserialization: {PATH}",
                       "PATH", filePath);
        }
        cereal::BinaryInputArchive archive(ifs);
        uint32_t sourceId;
        uint64_t dumpSize;
        std::string originId;
        originatorTypes originType;
        uint64_t dumpCompletedTime;
        uint64_t dumpElapsed;
        uint64_t dumpStartTime;
        archive(sourceId, dumpSize, originId, originType, dumpCompletedTime,
                dumpElapsed, dumpStartTime);
        sourceDumpId(sourceId);
        size(dumpSize);
        originatorId(originId);
        originatorType(originType);
        completedTime(dumpCompletedTime);
        elapsed(dumpElapsed);
        startTime(dumpStartTime);
        status(OperationStatus::Completed);
        dumpRequestStatus(HostResponse::Success);
    }
    catch (const std::exception& e)
    {
        lg2::error("Deserialization error: {PATH}, {ERROR}", "PATH", filePath,
                   "ERROR", e);
    }
}

} // namespace resource
} // namespace dump
} // namespace openpower
