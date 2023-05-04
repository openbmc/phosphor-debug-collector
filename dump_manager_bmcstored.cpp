#include "config.h"

#include "bmc_dump_entry.hpp"
#include "dump_internal.hpp"
#include "dump_manager_bmc.hpp"
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
namespace bmc_stored
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

uint64_t timeToEpoch(std::string timeStr)
{
    std::tm t{};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&t, "%Y%m%d%H%M%S");
    if (ss.fail())
    {
        // Dump will not be listed if the time is not format
        // but to get the dump listed, setting the current time.
        log<level::ERR>(
            fmt::format("Invalid human readable time value({})", timeStr)
                .c_str());
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
    return mktime(&t);
}

void Manager::createEntry(const std::filesystem::path& file)
{
    std::regex file_regex(dumpFilenameFormat.c_str());

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
    auto ts = match[EPOCHTIME_POS];

    uint64_t timestamp = 1000 * 1000;
    if (TIMESTAMP_FORMAT == 1)
    {
        timestamp *= timeToEpoch(ts);
    }
    else
    {
        timestamp *= stoull(ts);
    }

    auto id = stoul(idString);

    // If there is an existing entry update it and return.
    auto dumpEntry = entries.find(id);
    if (dumpEntry != entries.end())
    {
        dynamic_cast<phosphor::dump::bmc_stored::Entry*>(
            dumpEntry->second.get())
            ->update(timestamp, std::filesystem::file_size(file), file);
        return;
    }

    // Entry Object path.
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

    try
    {
        createEntry(id, objPath, timestamp, std::filesystem::file_size(file),
                    file, phosphor::dump::OperationStatus::Completed,
                    std::string(), originatorTypes::Internal);
    }
    catch (const InternalFailure& e)
    {
        log<level::ERR>(
            fmt::format(
                "Error in creating dump entry, errormsg({}), OBJECTPATH({}), "
                "ID({}), TIMESTAMP({}), SIZE({}), FILENAME({})",
                e.what(), objPath.c_str(), id, timestamp,
                std::filesystem::file_size(file), file.filename().c_str())
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
            lastEntryId = std::max(lastEntryId,
                                   static_cast<uint32_t>(std::stoul(idStr)));
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

    size = (size > allocatedSize ? 0 : allocatedSize - size);

#ifdef BMC_DUMP_ROTATE_CONFIG
    // Delete the first existing file until the space is enough
    while (size < minDumpSize)
    {
        auto delEntry = min_element(entries.begin(), entries.end(),
                                    [](const auto& l, const auto& r) {
            return l.first < r.first;
        });
        auto delPath = std::filesystem::path(dumpDir) /
                       std::to_string(delEntry->first);

        size += getDirectorySize(delPath);

        delEntry->second->delete_();
    }
#else
    using namespace sdbusplus::xyz::openbmc_project::Dump::Create::Error;
    using Reason = xyz::openbmc_project::Dump::Create::QuotaExceeded::REASON;

    if (size < minDumpSize)
    {
        log<level::ERR>(fmt::format("Not enough space available({}) miniumum "
                                    "needed({}) filled({}) allocated({})",
                                    size, minDumpSize,
                                    getDirectorySize(dumpDir), allocatedSize)
                            .c_str());
        // Reached to maximum limit
        elog<QuotaExceeded>(Reason("Not enough space: Delete old dumps"));
    }
#endif

    if (size > maxDumpSize)
    {
        size = maxDumpSize;
    }

    return size;
}

} // namespace bmc_stored
} // namespace dump
} // namespace phosphor
