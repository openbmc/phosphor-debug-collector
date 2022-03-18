#include "config.h"

#include "dump_helper.hpp"

#include "dump_manager.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

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
namespace util
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
        throw std::runtime_error{"Invalid human readable time value"};
    }
    return mktime(&t);
}

void DumpHelper::createEntry(const std::filesystem::path& file)
{
    std::regex file_regex(dumpFilenameFormat.c_str());

    std::smatch match;
    std::string name = file.filename();

    if (!(std::regex_search(name, match, file_regex)) || match.empty())
    {
        lg2::error("Invalid Dump file name, FILENAME: {FILENAME}", "FILENAME",
                   file.filename());
        return;
    }

    auto idString = match[FILENAME_DUMP_ID_POS];
    auto ts = match[FILENAME_EPOCHTIME_POS];

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

    auto dumpEntryOpt = mgr->getEntryById(id);
    if (dumpEntryOpt)
    {
        auto& dumpEntry = *dumpEntryOpt;
        dumpEntry.get()->update(timestamp, std::filesystem::file_size(file),
                                file);
    }

    // Entry Object path.
    auto objPath = mgr->getEntryObjectPath(id);

    try
    {
        mgr->createEntry(id, objPath, timestamp,
                         std::filesystem::file_size(file), file,
                         phosphor::dump::OperationStatus::Completed,
                         std::string(), originatorTypes::Internal);
    }
    catch (const InternalFailure& e)
    {
        lg2::error(
            "Error in creating dump entry, errormsg: {ERROR_MSG}, OBJECTPATH: {OBJECT_PATH}, "
            "ID: {ID}, TIMESTAMP {TIMESTAMP}, SIZE: {SIZE}, FILENAME: {FILENAME}",
            "ERROR_MSG", e.what(), "OBJECT_PATH", objPath.c_str(), "ID", id,
            "TIMESTAMP", timestamp, "SIZE", std::filesystem::file_size(file),
            "FILENAME", file.filename().c_str());
        return;
    }
}

void DumpHelper::watchCallback(const UserMap& fileInfo)
{
    for (const auto& [file, event] : fileInfo)
    {
        // For any new dump file create dump entry object
        // and associated inotify watch.
        if (IN_CLOSE_WRITE == event)
        {
            if (!std::filesystem::is_directory(file))
            {
                // Don't require filename to be passed, as the path
                // of dump directory is stored in the childWatchMap
                removeWatch(file.parent_path());

                // dump file is written now create D-Bus entry
                createEntry(file);
            }
            else
            {
                removeWatch(file);
            }
        }
        // Start inotify watch on newly created directory.
        else if ((IN_CREATE == event) && std::filesystem::is_directory(file))
        {
            auto watchObj = std::make_unique<Watch>(
                eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE, EPOLLIN, file,
                std::bind(std::mem_fn(
                              &phosphor::dump::util::DumpHelper::watchCallback),
                          this, std::placeholders::_1));

            childWatchMap.emplace(file, std::move(watchObj));
        }
    }
}

void DumpHelper::removeWatch(const std::filesystem::path& path)
{
    // Delete Watch entry from map.
    childWatchMap.erase(path);
}

void DumpHelper::restore()
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
            mgr->updateLastEntryId(
                std::max(mgr->getLastEntryId(),
                         static_cast<uint32_t>(std::stoul(idStr))));
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

size_t DumpHelper::getAllowedSize()
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
        auto optionalEntry = mgr->getEntryWithLowestId();
        if (optionalEntry.has_value())
        {
            phosphor::dump::entry* entry = optionalEntry.value();
            if (!entry->path().empty())
            {
                size += getDirectorySize(entry->path());
            }

            entry->delete_();
        }
    }
#else
    using namespace sdbusplus::xyz::openbmc_project::Dump::Create::Error;
    using Reason = xyz::openbmc_project::Dump::Create::QuotaExceeded::REASON;

    if (size < minDumpSize)
    {
        lg2::error("Not enough space available: {AVAILABLE} miniumum "
                   "needed: {MINIMUM} filled: {FILLED} allocated: {ALLOCATED}",
                   "AVAILABLE", size, "MINIMUM", minDumpSize, "FILLED",
                   getDirectorySize(dumpDir), "ALLOCATED", allocatedSize);
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

} // namespace util
} // namespace dump
} // namespace phosphor
