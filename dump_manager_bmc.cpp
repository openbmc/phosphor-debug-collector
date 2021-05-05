#include "config.h"

#include "dump_manager_bmc.hpp"

#include "bmcstored_dump_entry.hpp"
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

    std::string dumpType = "user";
    std::string type = extractParameter<std::string>(
        convertCreateParametersToString(CreateParameters::DumpType), params);
    if (!type.empty())
    {
        std::string dumpType = validateDumpType(type, params);
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

    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    createEntry(id, objPath, timeStamp, 0, std::string(),
                phosphor::dump::OperationStatus::InProgress, originatorId,
                originatorType);
    if (dumpType == "user")
    {
        Manager::fUserDumpInProgress = true;
    }
    return objPath.string();
}

void Manager::createEntry(const uint32_t id, const std::string objPath,
                          const uint64_t ms, uint64_t size,
                          const std::filesystem::path& file,
                          phosphor::dump::OperationStatus status,
                          std::string originatorId,
                          originatorTypes originatorType)
{
    try
    {
        entries.emplace(
            id, std::make_unique<bmc_stored::Entry<
                    sdbusplus::xyz::openbmc_project::Dump::Entry::server::BMC>>(
                    bus, objPath.c_str(), id, ms, size, file, status,
                    originatorId, originatorType, *this));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error("Error in creating dump entry, errormsg: {ERROR}, "
                   "OBJECTPATH: {OBJECT_PATH}, ID: {ID}",
                   "ERROR", e, "OBJECT_PATH", objPath, "ID", id);
        elog<InternalFailure>();
    }
}

uint32_t Manager::captureDump(std::string& type, const std::string& path)
{
    // Get Dump size.
    auto size = dumpHelper.getAllowedSize();

    pid_t pid = fork();

    if (pid == 0)
    {
        std::filesystem::path dumpPath(dumpHelper.dumpDir);
        auto id = std::to_string(lastEntryId + 1);
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
            childPtrMap.erase(pid);
        };
        try
        {
            childPtrMap.emplace(
                pid, std::make_unique<Child>(dumpHelper.eventLoop.get(), pid,
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
    return ++lastEntryId;
}

} // namespace bmc
} // namespace dump
} // namespace phosphor
