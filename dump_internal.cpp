#include "config.h"

#include "dump_internal.hpp"

#include "dump_manager_bmc.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <sys/inotify.h>
#include <unistd.h>

#include <ctime>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

namespace phosphor
{
namespace dump
{
namespace internal
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

std::map<Type, DumpInfo> Manager::typeMap = {};

void Manager::create(Type type, std::vector<std::string> fullPaths)
{
    captureDump(type, fullPaths);
}

uint32_t Manager::captureDump(Type type,
                              const std::vector<std::string>& fullPaths)
{
    auto itr = typeMap.find(type);
    if (itr == typeMap.end())
    {
        log<level::ERR>("Invaid dump type", entry("TYPE=%d", type));
        report<InternalFailure>();
    }

    phosphor::dump::bmc::Manager& mngr =
        reinterpret_cast<phosphor::dump::bmc::Manager&>(itr->second.manager);
    // Get Dump size.
    auto size = mngr.getAllowedSize();

    pid_t pid = fork();
    if (pid == 0)
    {
        fs::path dumpPath(mngr.dumpDir);
        auto id = std::to_string(mngr.getLastEntryId() + 1);
        dumpPath /= id;

        // get dreport type map entry
        auto tempType = typeMap.find(type);
        execl("/usr/bin/dreport", "dreport", "-d", dumpPath.c_str(), "-i",
              id.c_str(), "-s", std::to_string(size).c_str(), "-q", "-v", "-p",
              fullPaths.empty() ? "" : fullPaths.front().c_str(), "-t",
              tempType->second.type.c_str(), nullptr);

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
    return mngr.getNextId();
}
} // namespace internal
} // namespace dump
} // namespace phosphor
