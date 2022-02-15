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

#include <cmath>
#include <ctime>
#include <regex>

namespace phosphor
{
namespace dump
{
namespace bmc
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;
using Unavailable = sdbusplus::xyz::openbmc_project::Common::Error::Unavailable;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

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
    if (params.size() > CREATE_DUMP_MAX_PARAMS)
    {
        log<level::WARNING>(
            "BMC dump accepts not more than 2 additional parameters");
    }

    if (Manager::fUserDumpInProgress == true)
    {
        elog<Unavailable>(Reason("User initiated dump is already in progress"));
    }
    Manager::fUserDumpInProgress = true;

    std::filesystem::path objPath;
    uint32_t id = 0;
    try
    {
        // Get the originator id and type from params
        std::string originatorId;
        originatorTypes originatorType;

        phosphor::dump::extractOriginatorProperties(params, originatorId,
                                                    originatorType);

        std::vector<std::string> paths;
        id = captureDump(Type::UserRequested, paths);

        // Entry Object path.
        objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

        uint64_t timeStamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

        entries.insert(std::make_pair(
            id, std::make_unique<bmc::Entry>(
                    bus, objPath.c_str(), id, timeStamp, 0, std::string(),
                    phosphor::dump::OperationStatus::InProgress, originatorId,
                    originatorType, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        Manager::fUserDumpInProgress = false;
        log<level::ERR>(fmt::format("Error in creating dump entry, "
                                    "errormsg({}), OBJECTPATH({}), ID({})",
                                    e.what(), objPath.c_str(), id)
                            .c_str());
        elog<InternalFailure>();
    }
    catch (const InternalFailure& ex)
    {
        Manager::fUserDumpInProgress = false;
        throw ex;
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
        log<level::ERR>(fmt::format("Error occurred during dreport "
                                    "function execution, errno({})",
                                    error)
                            .c_str());
        elog<InternalFailure>();
    }
    else if (pid > 0)
    {
        // local variable goes out of scope using pointer, callback method
        // need to dellocate the pointer
        Type* typePtr = new Type();
        *typePtr = type;
        int rc =
            sd_event_add_child(eventLoop.get(), nullptr, pid,
                               WEXITED | WSTOPPED, callback, (void*)(typePtr));
        if (0 > rc)
        {
            // Failed to add to event loop
            log<level::ERR>(fmt::format("Error occurred during the "
                                        "sd_event_add_child call, rc({})",
                                        rc)
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

void Manager::createEntry(const std::filesystem::path& file)
{
    // Dump File Name format obmcdump_ID_EPOCHTIME.EXT
    static constexpr auto ID_POS = 1;
    static constexpr auto EPOCHTIME_POS = 2;
    std::regex file_regex("obmcdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)");

    std::smatch match;
    std::string name = file.filename();

    if (!((std::regex_search(name, match, file_regex)) && (match.size() > 0)))
    {
        log<level::ERR>(fmt::format("Invalid Dump file name, FILENAME({})",
                                    file.filename().c_str())
                            .c_str());
        return;
    }

    auto idString = match[ID_POS];
    uint64_t timestamp = stoull(match[EPOCHTIME_POS]) * 1000 * 1000;

    auto id = stoul(idString);

    // If there is an existing entry update it and return.
    auto dumpEntry = entries.find(id);
    if (dumpEntry != entries.end())
    {
        dynamic_cast<phosphor::dump::bmc::Entry*>(dumpEntry->second.get())
            ->update(timestamp, std::filesystem::file_size(file), file);
        return;
    }

    // Entry Object path.
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

    // TODO: Get the persisted originator id & type
    // For now, replacing it with null
    try
    {
        entries.insert(std::make_pair(
            id, std::make_unique<bmc::Entry>(
                    bus, objPath.c_str(), id, timestamp,
                    std::filesystem::file_size(file), file,
                    phosphor::dump::OperationStatus::Completed, std::string(),
                    originatorTypes::Internal, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(
            fmt::format("Error in creating dump entry, errormsg({}), "
                        "OBJECTPATH({}), "
                        "ID({}), TIMESTAMP({}), SIZE({}), FILENAME({})",
                        e.what(), objPath.c_str(), id, timestamp,
                        std::filesystem::file_size(file),
                        file.filename().c_str())
                .c_str());
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
            if (!std::filesystem::is_directory(i.first))
            {
                // Don't require filename to be passed, as the path
                // of dump directory is stored in the childWatchMap
                removeWatch(i.first.parent_path());

                // dump file is written now create D-Bus entry
                createEntry(i.first);
            }
            else
            {
                removeWatch(i.first);
            }
        }
        // Start inotify watch on newly created directory.
        else if ((IN_CREATE == i.second) &&
                 std::filesystem::is_directory(i.first))
        {
            auto watchObj = std::make_unique<Watch>(
                eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE, EPOLLIN, i.first,
                std::bind(
                    std::mem_fn(&phosphor::dump::bmc::Manager::watchCallback),
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

size_t getDirectorySize(const std::string dir)
{
    auto size = 0;
    for (const auto& p : std::filesystem::recursive_directory_iterator(dir))
    {
        if (!std::filesystem::is_directory(p))
        {
            size += std::ceil(std::filesystem::file_size(p) / 1024.0);
        }
    }
    return size;
}

size_t Manager::getAllowedSize()
{
    // Get current size of the dump directory.
    auto size = getDirectorySize(dumpDir);

    // Set the Dump size to Maximum  if the free space is greater than
    // Dump max size otherwise return the available size.

    size = (size > BMC_DUMP_TOTAL_SIZE ? 0 : BMC_DUMP_TOTAL_SIZE - size);

#ifdef BMC_DUMP_ROTATE_CONFIG
    // Delete the first existing file until the space is enough
    while (size < BMC_DUMP_MIN_SPACE_REQD)
    {
        auto delEntry = min_element(
            entries.begin(), entries.end(),
            [](const auto& l, const auto& r) { return l.first < r.first; });
        auto delPath =
            std::filesystem::path(dumpDir) / std::to_string(delEntry->first);

        size += getDirectorySize(delPath);

        delEntry->second->delete_();
    }
#else
    using namespace sdbusplus::xyz::openbmc_project::Dump::Create::Error;
    using Reason = xyz::openbmc_project::Dump::Create::QuotaExceeded::REASON;

    if (size < BMC_DUMP_MIN_SPACE_REQD)
    {
        // Reached to maximum limit
        elog<QuotaExceeded>(Reason("Not enough space: Delete old dumps"));
    }
#endif

    if (size > BMC_DUMP_MAX_SIZE)
    {
        size = BMC_DUMP_MAX_SIZE;
    }

    return size;
}

} // namespace bmc
} // namespace dump
} // namespace phosphor
