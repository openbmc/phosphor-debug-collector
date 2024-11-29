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
    auto bus = sdbusplus::bus::new_default();
    // Log PEL for dump offload
    phosphor::dump::createPELOnDumpActions(
        bus, file, "Resource Dump", std::format("{:08x}", id),
        "xyz.openbmc_project.Logging.Entry.Level.Informational",
        "xyz.openbmc_project.Dump.Error.Offload");
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

    removeSerializedEntry();
    auto bus = sdbusplus::bus::new_default();
    // Log PEL for dump delete
    phosphor::dump::createPELOnDumpActions(
        bus, file, "Resource Dump", std::format("{:08x}", id),
        "xyz.openbmc_project.Logging.Entry.Level.Informational",
        "xyz.openbmc_project.Dump.Error.Invalidate");
    phosphor::dump::Entry::delete_();
}

template <typename Derived>
void Entry<Derived>::serialize()
{
    std::string idStr = std::format("{:08X}", id);
    std::filesystem::path dir =
        std::filesystem::path(openpower::dump::OP_DUMP_PATH) / idStr /
        phosphor::dump::PRESERVE;
    std::filesystem::path serializedFilePath =
        dir / phosphor::dump::SERIAL_FILE;

    try
    {
        if (!std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir);
        }

        std::ofstream ofs(serializedFilePath, std::ios::binary);
        if (!ofs.is_open())
        {
            lg2::error("Failed to open file for serialization: {PATH}", "PATH",
                       serializedFilePath);
        }

        nlohmann::json j;
        j["version"] = CLASS_SERIALIZATION_VERSION;
        j["dumpId"] = id;
        j["sourceDumpId"] = getSourceDumpId();
        j["size"] = size();
        j["originatorId"] = originatorId();
        j["originatorType"] = originatorType();
        j["completedTime"] = completedTime();
        j["elapsed"] = elapsed();
        j["startTime"] = startTime();

        nlohmann::json::to_cbor(j, ofs);
    }
    catch (const std::exception& e)
    {
        lg2::error("Serialization error: {PATH} {ERROR}", "PATH",
                   serializedFilePath, "ERROR", e.what());
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
}

template <typename Derived>
void Entry<Derived>::deserialize(const std::filesystem::path& dumpPath)
{
    try
    {
        std::filesystem::path dir = dumpPath / phosphor::dump::PRESERVE;

        if (!std::filesystem::exists(dir))
        {
            lg2::info("Serialization directory: {SERIAL_DIR} doesnt exist, "
                      "skip deserialization",
                      "SERIAL_DIR", dir);
            return;
        }

        std::filesystem::path serializedFilePath =
            dir / phosphor::dump::SERIAL_FILE;
        std::ifstream ifs(serializedFilePath, std::ios::binary);
        if (!ifs.is_open())
        {
            lg2::error("Failed to open file for deserialization: {PATH}",
                       "PATH", serializedFilePath);
            return;
        }

        nlohmann::json j = nlohmann::json::from_cbor(ifs);

        if (j["version"] == CLASS_SERIALIZATION_VERSION)
        {
            uint32_t stored_id = j["dumpId"].get<uint32_t>();

            if (stored_id == id)
            {
                setSourceDumpId(j["sourceDumpId"].get<uint32_t>());
                size(j["size"].get<uint64_t>());
                originatorId(j["originatorId"].get<std::string>());
                originatorType(
                    j["originatorType"].get<phosphor::dump::originatorTypes>());
                completedTime(j["completedTime"].get<uint64_t>());
                elapsed(j["elapsed"].get<uint64_t>());
                startTime(j["startTime"].get<uint64_t>());
            }
            else
            {
                lg2::error(
                    "The id in the serial file: {ID_IN_FILE} is not "
                    "matching with dumpid: {DUMPID}, skipping deserialization",
                    "ID_IN_FILE", stored_id, "ID", id);
            }
        }
        else
        {
            lg2::error("The serialized file version and current class version"
                       "dont match, skipping deserialization {FILE_VERSION}",
                       "FILE_VERSION", j["version"].get<uint32_t>());
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Deserialization error: {PATH}, {ERROR}", "PATH", dumpPath,
                   "ERROR", e.what());
    }
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

    serialize();
}

template <typename Derived>
void Entry<Derived>::removeSerializedEntry()
{
    std::string idStr = std::format("{:08X}", getDumpId());
    const std::filesystem::path serializedDir =
        std::filesystem::path(openpower::dump::OP_DUMP_PATH) / idStr;

    // Check if the directory exists, return if it doesn't
    if (!std::filesystem::exists(serializedDir))
    {
        return;
    }

    try
    {
        std::filesystem::remove_all(serializedDir);
        lg2::info("Successfully removed directory: {PATH}", "PATH",
                  serializedDir);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Log Error message and continue
        lg2::error("Failed to delete directory, path: {PATH} errormsg: {ERROR}",
                   "PATH", serializedDir, "ERROR", e);
    }
}

} // namespace openpower::dump::host
