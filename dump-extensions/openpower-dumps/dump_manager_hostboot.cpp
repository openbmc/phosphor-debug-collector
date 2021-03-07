#include "config.h"

#include "dump_manager_hostboot.hpp"

#include "hostboot_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <sys/inotify.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

#include <ctime>
#include <regex>

namespace openpower
{
namespace dump
{
namespace hostboot
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

constexpr auto INVALID_DUMP_SIZE = 0;

sdbusplus::message::object_path
    Manager::createDump(std::map<std::string, std::string> params)
{
    if (!params.empty())
    {
        log<level::WARNING>("Hostboot dump accepts no additional parameters");
    }

    uint32_t id = ++lastEntryId;
    // Entry Object path.
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

    try
    {
        std::time_t timeStamp = std::time(nullptr);
        entries.insert(std::make_pair(
            id, std::make_unique<openpower::dump::hostboot::Entry>(
                    bus, objPath.c_str(), id, timeStamp, INVALID_DUMP_SIZE,
                    std::string(), phosphor::dump::OperationStatus::InProgress,
                    *this)));
    }
    catch (const InternalFailure& e)
    {
        // Trace additional details and log internal failure.
        log<level::ERR>(e.what());
        log<level::ERR>("Error in creating dump entry",
                        entry("OBJECTPATH=%s", objPath.c_str()),
                        entry("ID=%d", id));
        elog<InternalFailure>();
    }

    return objPath.string();
}

void Manager::notify(uint32_t dumpId, uint64_t)
{
    std::vector<std::string> paths = {};
    auto dumpPath = std::filesystem::path(HOSTBOOT_DUMP_TMP_FILE_DIR) /
                    std::to_string(dumpId);
    paths.push_back(dumpPath.string());

    captureDump(paths, dumpId);
}

uint32_t Manager::captureDump(const std::vector<std::string>& fullPaths,
                              uint32_t id)
{
    // Get Dump size.
    // TODO #ibm-openbmc/issues/3061
    // Dump request will be rejected if there is not enough space told
    // one complete dump, change this behavior to crate a partial dump
    // with available space.
    auto size = getAllowedSize();

    pid_t pid = fork();

    if (pid == 0)
    {
        std::filesystem::path dumpPath(dumpDir);
        auto idStr = std::to_string(id);
        dumpPath /= idStr;

        execl("/usr/bin/opdreport", "opdreport", "-d", dumpPath.c_str(), "-i",
              idStr.c_str(), "-s", std::to_string(size).c_str(), "-q", "-v",
              "-p", fullPaths.empty() ? "" : fullPaths.front().c_str(), "-t",
              "hostboot", "-n", "hbdump", nullptr);

        // opdreport script execution is failed.
        auto error = errno;
        log<level::ERR>(
            "Hostboot Dump: Error occurred during opdreport function execution",
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
            log<level::ERR>("Hostboot Dump: Error occurred during the "
                            "sd_event_add_child call",
                            entry("RC=%d", rc));
            elog<InternalFailure>();
        }
    }
    else
    {
        auto error = errno;
        log<level::ERR>("Hostboot Dump: Error occurred during fork",
                        entry("ERRNO=%d", error));
        elog<InternalFailure>();
    }

    return 0;
}

} // namespace hostboot
} // namespace dump
} // namespace openpower
