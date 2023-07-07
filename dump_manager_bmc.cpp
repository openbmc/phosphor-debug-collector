#include "config.h"

#include "dump_manager_bmc.hpp"

#include "dump_types.hpp"
#include "dump_entry_handler.hpp"
#include "new_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"
#include "xyz/openbmc_project/Dump/Entry/BMC/server.hpp"

#include <sys/inotify.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/source/base.hpp>

#include <cmath>
#include <ctime>

namespace phosphor
{
namespace dump
{
namespace bmc
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

bool Manager::fUserDumpInProgress = false;
constexpr auto BMC_DUMP = "BMC_DUMP";

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    if (params.size() > CREATE_DUMP_MAX_PARAMS)
    {
        lg2::warning("BMC dump accepts not more than 2 additional parameters");
    }

    // Get the originator id and type from params
    std::string originatorId;
    originatorTypes originatorType;

    phosphor::dump::extractOriginatorProperties(params, originatorId,
                                                originatorType);
    using CreateParameters =
        sdbusplus::common::xyz::openbmc_project::dump::Create::CreateParameters;

    DumpTypes dumpType = DumpTypes::USER;
    std::string type = extractParameter<std::string>(
        convertCreateParametersToString(CreateParameters::DumpType), params);
    if (!type.empty())
    {
        dumpType = validateDumpType(type, BMC_DUMP);
    }

    if (dumpType == DumpTypes::ELOG)
    {
        dumpType = getErrorDumpType(params);
    }
    std::string path = extractParameter<std::string>(
        convertCreateParametersToString(CreateParameters::FilePath), params);

    if ((Manager::fUserDumpInProgress == true) && (dumpType == DumpTypes::USER))
    {
        lg2::info("Another user initiated dump in progress");
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    lg2::info("Initiating new BMC dump with type: {TYPE} path: {PATH}", "TYPE",
              dumpTypeToString(dumpType).value(), "PATH", path);

    auto id = captureDump(dumpType, path);
    std::filesystem::path objPath;

    try
    {
        uint64_t timeStamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        objPath = createEntry(id, timeStamp, 0, std::string(),
                              phosphor::dump::OperationStatus::InProgress,
                              originatorId, originatorType);
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error("Error in creating dump entry, errormsg: {ERROR}, "
                   "ID: {ID}",
                   "ERROR", e, "ID", id);
        elog<InternalFailure>();
    }

    if (dumpType == DumpTypes::USER)
    {
        Manager::fUserDumpInProgress = true;
    }
    return objPath.string();
}

uint32_t Manager::captureDump(DumpTypes type, const std::string& path)
{
    using ::sdeventplus::source::Child;
    // Get Dump size.
    auto size = getAllowedSize(dumpDir, BMC_DUMP_MAX_SIZE,
                               BMC_DUMP_MIN_SPACE_REQD, BMC_DUMP_TOTAL_SIZE);

    std::filesystem::path dumpPath(dumpDir);
    auto id = std::to_string(lastEntryId + 1);
    dumpPath /= id;
    std::vector<std::string> commandAndArgs{"/usr/bin/dreport",
                                        "-d",
                                        dumpPath.string(),
                                        "-i",
                                        id,
                                        "-s",
                                        std::to_string(size),
                                        "-q",
                                        "-v",
                                        "-p",
                                        path.empty() ? "\"\"" : path,
                                        "-t",
                                        dumpTypeToString(type).value()};

    std::function<void()> userCallback;
    if (type == DumpTypes::USER)
    {
        userCallback = []() {
            lg2::info("User initiated dump completed, resetting flag");
            Manager::fUserDumpInProgress = false;
        };
    }
    dumpCollector.collectDump(commandAndArgs, std::move(userCallback));
    return ++lastEntryId;
}

void Manager::erase(uint32_t entryId)
{
    lg2::error("> dump manager erase");
    entries.erase(entryId);
    lg2::error("< dump manager erase");
}

void Manager::deleteAll()
{
    auto iter = entries.begin();
    while (iter != entries.end())
    {
        auto& entry = iter->second;
        ++iter;
        entry->delete_();
    }
}

std::filesystem::path
    Manager::createEntry(const uint32_t id, const uint64_t ms,

                         uint64_t fileSize, const std::filesystem::path& file,
                         phosphor::dump::OperationStatus status,
                         std::string originatorId,
                         OriginatorTypes originatorType)
{
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);
    lastEntryId = std::max(lastEntryId, id);
    try
    {
        entries.emplace(
            id, std::make_unique<phosphor::dump::new_::Entry<
                    sdbusplus::xyz::openbmc_project::Dump::Entry::server::BMC,
                    phosphor::dump::DumpEntryHelper>>(
                    bus, objPath.c_str(), id, ms, fileSize, file, status,
                    originatorId, originatorType, *this));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error(
            "Error in creating dump entry, errormsg: {ERROR}, "
            "OBJECTPATH: {OBJECT_PATH}, ID: {ID}, TIMESTAMP: {TIMESTAMP}, "
            "SIZE: {SIZE}, FILENAME: {FILENAME}",
            "ERROR", e, "OBJECT_PATH", objPath, "ID", id, "TIMESTAMP", ms,
            "SIZE", fileSize, "FILENAME", file);
        throw;
    }
    return objPath;
}

std::filesystem::path Manager::createOrUpdateEntry(
    const uint32_t id, const uint64_t timestamp, uint64_t fileSize,
    const std::filesystem::path& file, phosphor::dump::OperationStatus status,
    std::string originatorId, OriginatorTypes originatorType)
{
    auto dumpEntry = getEntry(id);
    if (dumpEntry != nullptr)
    {
        dumpEntry->markComplete(timestamp, std::filesystem::file_size(file),
                                file);
    }
    else
    {
        return createEntry(id, timestamp, fileSize, file, status, originatorId,
                           originatorType);
    }
    return std::filesystem::path(baseEntryPath) / std::to_string(id);
}
} // namespace bmc
} // namespace dump
} // namespace phosphor
