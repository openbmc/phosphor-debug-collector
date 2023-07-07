#include "config.h"

#include "dump_manager_bmc.hpp"

#include "dump_entry.hpp"
#include "dump_entry_helper.hpp"
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
    OriginatorTypes originatorType;

    phosphor::dump::extractOriginatorProperties(params, originatorId,
                                                originatorType);
    using CreateParameters =
        sdbusplus::common::xyz::openbmc_project::dump::Create::CreateParameters;

    std::string dumpType = "user";
    std::string type = extractParameter<std::string>(
        convertCreateParametersToString(CreateParameters::DumpType), params);
    if (!type.empty())
    {
        dumpType = validateDumpType(type, BMC_DUMP);
    }

    if (dumpType == "elog")
    {
        dumpType = getErrorDumpType(params);
    }
    std::string path = extractParameter<std::string>(
        convertCreateParametersToString(CreateParameters::FilePath), params);

    if ((Manager::fUserDumpInProgress == true) && (dumpType == "user"))
    {
        log<level::ERR>("Another user initiated dump in progress");
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    lg2::info("Initiating new BMC dump with type: {TYPE} path: {PATH}", "TYPE",
              dumpType, "PATH", path);

    auto id = captureDump(dumpType, path);

    // Entry Object path.
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

    try
    {
        uint64_t timeStamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

        entries.emplace(
            id,
            std::make_unique<
                Entry<sdbusplus::xyz::openbmc_project::Dump::Entry::server::BMC,
                      DumpEntryHelper>>(
                bus, objPath.c_str(), id, timeStamp, 0, std::string(),
                phosphor::dump::OperationStatus::InProgress, originatorId,
                originatorType, *this));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error("Error in creating dump entry, errormsg: {ERROR}, "
                   "OBJECTPATH: {OBJECT_PATH}, ID: {ID}",
                   "ERROR", e, "OBJECT_PATH", objPath, "ID", id);
        elog<InternalFailure>();
    }

    if (dumpType == "user")
    {
        Manager::fUserDumpInProgress = true;
    }
    return objPath.string();
}

uint32_t Manager::captureDump(const std::string& type, const std::string& path)
{
    // Get Dump size.
    auto size = getAllowedSize(dumpDir, BMC_DUMP_MAX_SIZE,
                               BMC_DUMP_MIN_SPACE_REQD, BMC_DUMP_TOTAL_SIZE);

    pid_t pid = fork();

    if (pid == 0)
    {
        std::filesystem::path dumpPath(dumpDir);
        auto id = std::to_string(currentEntryId() + 1);
        dumpPath /= id;

        execl("/usr/bin/dreport", "dreport", "-d", dumpPath.c_str(), "-i",
              id.c_str(), "-s", std::to_string(size).c_str(), "-q", "-v", "-p",
              path.empty() ? "" : path.c_str(), "-t", type.c_str(), nullptr);

        // dreport script execution is failed.
        auto error = errno;
        lg2::error("Error occurred during dreport function execution, "
                   "errno: {ERRNO}",
                   "ERRNO", error);
        elog<InternalFailure>();
    }
    else if (pid > 0)
    {
        Child::Callback callback = [this, type, pid](Child&, const siginfo_t*) {
            if (type == "user")
            {
                lg2::info("User initiated dump completed, resetting flag");
                Manager::fUserDumpInProgress = false;
            }
            this->childPtrMap.erase(pid);
        };
        try
        {
            childPtrMap.emplace(pid,
                                std::make_unique<Child>(eventLoop.get(), pid,
                                                        WEXITED | WSTOPPED,
                                                        std::move(callback)));
        }
        catch (const sdeventplus::SdEventError& ex)
        {
            // Failed to add to event loop
            lg2::error(
                "Error occurred during the sdeventplus::source::Child creation "
                "ex: {ERROR}",
                "ERROR", ex);
            elog<InternalFailure>();
        }
    }
    else
    {
        auto error = errno;
        lg2::error("Error occurred during fork, errno: {ERRNO}", "ERRNO",
                   error);
        elog<InternalFailure>();
    }
    return incrementLastEntryId();
}

void Manager::createEntry(const uint32_t id, const uint64_t ms,
                          uint64_t fileSize, const std::filesystem::path& file,
                          phosphor::dump::OperationStatus status,
                          std::string originatorId,
                          OriginatorTypes originatorType)
{
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);
    try
    {
        entries.emplace(
            id,
            std::make_unique<
                Entry<sdbusplus::xyz::openbmc_project::Dump::Entry::server::BMC,
                      DumpEntryHelper>>(bus, objPath.c_str(), id, ms, fileSize,
                                        file, status, originatorId,
                                        originatorType, *this));
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
}

} // namespace bmc
} // namespace dump
} // namespace phosphor
