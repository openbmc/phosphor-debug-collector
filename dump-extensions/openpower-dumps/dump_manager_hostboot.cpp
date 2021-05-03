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

void Manager::createEntry(const std::filesystem::path& file)
{
    // Dump File Name format hbdump_ID_EPOCHTIME.EXT
    static constexpr auto ID_POS = 1;
    static constexpr auto EPOCHTIME_POS = 2;
    std::regex file_regex("hbdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)");

    std::smatch match;
    std::string name = file.filename();

    if (!((std::regex_search(name, match, file_regex)) && (match.size() > 0)))
    {
        log<level::ERR>("Invalid Dump file name",
                        entry("FILENAME=%s", file.filename().c_str()));
        return;
    }

    auto idString = match[ID_POS];
    auto msString = match[EPOCHTIME_POS];

    auto id = stoul(idString);

    // If there is an existing entry update it and return.
    auto dumpEntry = entries.find(id);
    if (dumpEntry != entries.end())
    {
        dynamic_cast<openpower::dump::hostboot::Entry*>(dumpEntry->second.get())
            ->update(stoull(msString), std::filesystem::file_size(file), file);
        return;
    }

    // Entry Object path.
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

    try
    {
        entries.insert(std::make_pair(
            id, std::make_unique<openpower::dump::hostboot::Entry>(
                    bus, objPath.c_str(), id, stoull(msString),
                    std::filesystem::file_size(file), file,
                    phosphor::dump::OperationStatus::Completed, *this)));
    }
    catch (const InternalFailure& e)
    {
        log<level::ERR>(e.what());
        log<level::ERR>("Error in creating hostboot dump entry",
                        entry("OBJECTPATH=%s", objPath.c_str()),
                        entry("ID=%d", id),
                        entry("TIMESTAMP=%ull", stoull(msString)),
                        entry("SIZE=%d", std::filesystem::file_size(file)),
                        entry("FILENAME=%s", file.c_str()));
        return;
    }
}

void Manager::watchCallback(const UserMap& fileInfo)
{
    for (const auto& i : fileInfo)
    {
        // For any new dump file create dump entry object
        // and associated inotify watch.
        if (IN_CLOSE_WRITE == i.second)
        {
            removeWatch(i.first);

            createEntry(i.first);
        }
        // Start inotify watch on newly created directory.
        else if ((IN_CREATE == i.second) &&
                 std::filesystem::is_directory(i.first))
        {
            auto watchObj = std::make_unique<Watch>(
                eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE, EPOLLIN, i.first,
                std::bind(
                    std::mem_fn(
                        &openpower::dump::hostboot::Manager::watchCallback),
                    this, std::placeholders::_1));

            childWatchMap.emplace(i.first, std::move(watchObj));
        }
    }
}

void Manager::removeWatch(const std::filesystem::path& path)
{
    // Delete Watch entry from map.
    childWatchMap.erase(path);
}

void Manager::restore()
{
    std::filesystem::path dir(dumpDir);
    if (!std::filesystem::exists(dir) || std::filesystem::is_empty(dir))
    {
        return;
    }

    // Dump file path: <DUMP_PATH>/<id>/<filename>
    for (const auto& p : std::filesystem::directory_iterator(dir))
    {
        auto idStr = p.path().filename().string();

        // Consider only directory's with dump id as name.
        // Note: As per design one file per directory.
        if ((std::filesystem::is_directory(p.path())) &&
            std::all_of(idStr.begin(), idStr.end(), ::isdigit))
        {
            lastEntryId =
                std::max(lastEntryId, static_cast<uint32_t>(std::stoul(idStr)));
            auto fileIt = std::filesystem::directory_iterator(p.path());
            // Create dump entry d-bus object.
            if (fileIt != std::filesystem::end(fileIt))
            {
                createEntry(fileIt->path());
            }
        }
    }
}

size_t Manager::getAllowedSize()
{
    using namespace sdbusplus::xyz::openbmc_project::Dump::Create::Error;
    using Reason = xyz::openbmc_project::Dump::Create::QuotaExceeded::REASON;

    auto size = 0;

    // Get current size of the dump directory.
    for (const auto& p : std::filesystem::recursive_directory_iterator(dumpDir))
    {
        if (!std::filesystem::is_directory(p))
        {
            size += std::filesystem::file_size(p);
        }
    }

    // Convert size into KB
    size = size / 1024;

    // Set the Dump size to Maximum  if the free space is greater than
    // Dump max size otherwise return the available size.

    size =
        (size > HOSTBOOT_DUMP_TOTAL_SIZE ? 0 : HOSTBOOT_DUMP_TOTAL_SIZE - size);

    if (size < HOSTBOOT_DUMP_MAX_SIZE)
    {
        // Reached to maximum limit
        elog<QuotaExceeded>(Reason("Not enough space: Delete old dumps"));
    }
    if (size > HOSTBOOT_DUMP_MAX_SIZE)
    {
        size = HOSTBOOT_DUMP_MAX_SIZE;
    }

    return size;
}

} // namespace hostboot
} // namespace dump
} // namespace openpower
