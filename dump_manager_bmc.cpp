#include "config.h"

#include "dump_manager_bmc.hpp"

#include "bmc_dump_entry.hpp"
#include "dump_internal.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <fmt/core.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
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

namespace internal
{

void Manager::create(Type type, std::vector<std::string> fullPaths)
{
    dumpMgr.phosphor::dump::bmc::Manager::captureDump(type, fullPaths);
}

} // namespace internal

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    if (params.size() > CREATE_BMC_DUMP_MAX_PARAMS)
    {
        log<level::WARNING>(
            fmt::format(
                "BMC dump accepts not more than ({}) additional parameters",
                CREATE_BMC_DUMP_MAX_PARAMS)
                .c_str());
    }

    if (Manager::fUserDumpInProgress == true)
    {
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    // Get the originator id and type from params
    std::string originatorId;
    originatorTypes originatorType;

    phosphor::dump::extractOriginatorProperties(params, originatorId,
                                                originatorType);

    std::vector<std::string> paths;

    Type dumpType = Type::UserRequested;
#ifdef FAULT_DATA_DUMP
    constexpr auto BMC_DUMP_TYPE =
        "xyz.openbmc_project.Dump.Internal.Create.Type";
    constexpr auto FAULT_DATA_DUMP_TYPE =
        "xyz.openbmc_project.Dump.Internal.Create.Type.FaultData";
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;
    auto iter = params.find(BMC_DUMP_TYPE);
    if (iter != params.end())
    {
        if (!std::holds_alternative<std::string>(iter->second))
        {
            log<level::ERR>("An invalid dump type passed");
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("BMC_DUMP_TYPE"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }

        auto strDumpType = std::get<std::string>(iter->second);
        // Only Fault data dump is supported
        if (strDumpType != FAULT_DATA_DUMP_TYPE)
        {
            log<level::ERR>(
                fmt::format("An invalid dump type passed ({})", strDumpType)
                    .c_str());
            elog<InvalidArgument>(
                Argument::ARGUMENT_NAME("BMC_DUMP_TYPE"),
                Argument::ARGUMENT_VALUE(strDumpType.c_str()));
        }
        dumpType = Type::FaultData;
    }
#endif
    auto id = captureDump(dumpType, paths);

    // Entry Object path.
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    createEntry(id, objPath, timeStamp, 0, std::string(),
                phosphor::dump::OperationStatus::InProgress, originatorId,
                originatorType);
    Manager::fUserDumpInProgress = true;
    return objPath.string();
}

void Manager::createEntry(const uint32_t id, const std::string objPath,
                          const uint64_t ms, uint64_t fileSize,
                          const std::filesystem::path& file,
                          phosphor::dump::OperationStatus status,
                          std::string originatorId,
                          originatorTypes originatorType)
{
    try
    {
        entries.insert(std::make_pair(
            id, std::make_unique<bmc::Entry>(
                    bus, objPath.c_str(), id, ms, fileSize, file, status,
                    originatorId, originatorType, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(fmt::format("Error in creating BMC dump entry, "
                                    "errormsg({}), OBJECTPATH({}), ID({})",
                                    e.what(), objPath.c_str(), id)
                            .c_str());
        elog<InternalFailure>();
    }
}

uint32_t Manager::captureDump(Type type,
                              const std::vector<std::string>& fullPaths)
{
    // Get Dump size.
    auto size = getAllowedSize();

    pid_t pid = fork();

    if (pid == 0)
    {
        std::filesystem::path dumpPath(dumpDir);
        auto id = std::to_string(lastEntryId + 1);

        dumpPath /= id;

        // get dreport type map entry
        auto tempType = TypeMap.find(type);
        execl("/usr/bin/dreport", "dreport", "-d", dumpPath.c_str(), "-i",
              id.c_str(), "-s", std::to_string(size).c_str(), "-q", "-v", "-p",
              fullPaths.empty() ? "" : fullPaths.front().c_str(), "-t",
              tempType->second.c_str(), nullptr);

        // dreport script execution is failed.
        auto error = errno;
        log<level::ERR>(fmt::format("Error occurred during dreport "
                                    "function execution, errno({})",
                                    error)
                            .c_str());
        elog<InternalFailure>();
    }
    else if (pid > 0)
    {
        Child::Callback callback = [this, type, pid](Child&, const siginfo_t*) {
            if (type == Type::UserRequested)
            {
                log<level::INFO>(
                    "User initiated dump completed, resetting flag");
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
            log<level::ERR>(
                fmt::format(
                    "Error occurred during the sdeventplus::source::Child "
                    "creation ex({})",
                    ex.what())
                    .c_str());
            elog<InternalFailure>();
        }
    }
    else
    {
        auto error = errno;
        log<level::ERR>(
            fmt::format("Error occurred during fork, errno({})", error)
                .c_str());
        elog<InternalFailure>();
    }
    return ++lastEntryId;
}

} // namespace bmc
} // namespace dump
} // namespace phosphor
