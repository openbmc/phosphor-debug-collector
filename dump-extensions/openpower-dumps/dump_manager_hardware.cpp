#include "config.h"

#include "dump_manager_hardware.hpp"

#include "hardware_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <fmt/core.h>
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
namespace hardware
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

constexpr auto INVALID_DUMP_SIZE = 0;

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    if (!params.empty())
    {
        log<level::WARNING>("Hardware dump accepts no additional parameters");
    }

    uint32_t id = ++lastEntryId;
    // Entry Object path.
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

    std::time_t timeStamp = std::time(nullptr);
    createEntry(entries.insert(
        std::make_pair(id, objPath, timeStamp, 0, std::string(),
                       phosphor::dump::OperationStatus::InProgress, *this)));

    return objPath.string();
}

void Manager::notify(uint32_t dumpId, uint64_t)
{
    std::vector<std::string> paths = {};
    auto dumpPath = std::filesystem::path(HARDWARE_DUMP_TMP_FILE_DIR) /
                    std::to_string(dumpId);
    paths.push_back(dumpPath.string());

    captureDump(paths, dumpId);
}

void Manager::createEntry(const uint32_t id, const std::string objPath,
                          const uint64_t ms, uint64_t fileSize,
                          const std::filesystem::path& file,
                          phosphor::dump::OperationStatus status)
{
    try
    {
        entries.insert(std::make_pair(
            id,
            std::make_unique<openpower::dump::hardware::Entry>(
                bus, objPath.c_str(), id, ms, fileSize, file, status, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(fmt::format("Error in creating hardware dump entry, "
                                    "errormsg({}), OBJECTPATH({}), ID({})",
                                    e.what(), objPath.c_str(), id)
                            .c_str());
        elog<InternalFailure>();
    }
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
              "hardware", "-n", "hwdump", nullptr);

        // opdreport script execution is failed.
        auto error = errno;
        log<level::ERR>(fmt::format("Hardware dump: Error occurred during "
                                    "opdreport function execution, errno({})",
                                    error)
                            .c_str());
        elog<InternalFailure>();
    }
    else if (pid > 0)
    {
        auto rc = sd_event_add_child(eventLoop.get(), nullptr, pid,
                                     WEXITED | WSTOPPED, callback, nullptr);
        if (0 > rc)
        {
            // Failed to add to event loop
            log<level::ERR>(fmt::format("Hardware dump: Error occurred during "
                                        "the sd_event_add_child call, rc({})",
                                        rc)
                                .c_str());
            elog<InternalFailure>();
        }
    }
    else
    {
        auto error = errno;
        log<level::ERR>(
            fmt::format("Hardware dump: Error occurred during fork, errno({})",
                        error)
                .c_str());
        elog<InternalFailure>();
    }

    return 0;
}

} // namespace hardware
} // namespace dump
} // namespace openpower
