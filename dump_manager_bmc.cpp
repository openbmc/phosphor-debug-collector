#include "config.h"

#include "dump_manager_bmc.hpp"

#include "bmc_dump_entry.hpp"
#include "dump_internal.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <sys/inotify.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

#include <ctime>

namespace phosphor
{
namespace dump
{
namespace bmc
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

namespace internal
{

void Manager::create(Type type, std::vector<std::string> fullPaths)
{
    dumpMgr.phosphor::dump::bmc::Manager::captureDump(type, fullPaths);
}

} // namespace internal

sdbusplus::message::object_path
    Manager::createDump(std::map<std::string, std::string> params)
{
    if (!params.empty())
    {
        log<level::WARNING>("BMC dump accepts no additional parameters");
    }
    std::vector<std::string> paths;
    auto id = captureDump(Type::UserRequested, paths);

    // Entry Object path.
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

    try
    {
        std::time_t timeStamp = std::time(nullptr);
        entries.insert(std::make_pair(
            id, std::make_unique<bmc::Entry>(
                    bus, objPath.c_str(), id, timeStamp, 0, std::string(),
                    phosphor::dump::OperationStatus::InProgress, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(e.what());
        log<level::ERR>("Error in creating dump entry",
                        entry("OBJECTPATH=%s", objPath.c_str()),
                        entry("ID=%d", id));
        elog<InternalFailure>();
    }

    return objPath.string();
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
        log<level::ERR>("Error occurred during dreport function execution",
                        entry("ERRNO=%d", error));
        elog<InternalFailure>();
    }
    else if (pid > 0)
    {
        auto rc = sd_event_add_child(eventLoop.get(), nullptr, pid,
                                     WEXITED | WSTOPPED, callback, nullptr);
        if (0 > rc)
        {
            // Failed to add to event loop
            log<level::ERR>("Error occurred during the sd_event_add_child call",
                            entry("RC=%d", rc));
            elog<InternalFailure>();
        }
    }
    else
    {
        auto error = errno;
        log<level::ERR>("Error occurred during fork", entry("ERRNO=%d", error));
        elog<InternalFailure>();
    }

    return ++lastEntryId;
}

} // namespace bmc
} // namespace dump
} // namespace phosphor
