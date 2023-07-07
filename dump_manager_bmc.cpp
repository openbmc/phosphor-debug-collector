#include "config.h"

#include "dump_manager_bmc.hpp"

#include "dump_entry_handler.hpp"
#include "dump_types.hpp"
#include "new_dump_entry.hpp"
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
#include <regex>

namespace phosphor
{
namespace dump
{
namespace bmc
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

bool Manager::fUserDumpInProgress = false;
constexpr auto BMC_DUMP = "BMC_DUMP";

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

    DumpTypes dumpType = DumpTypes::USER;
    std::string type = extractParameter<std::string>(
        convertCreateParametersToString(CreateParameters::DumpType), params);
    if (!type.empty())
    {
        dumpType = validateDumpType(type, BMC_DUMP);
    }

    if (dumpType == DumpTypes::ELOG)
    {
        dumpType = getErrorDumpType(params);
    }
    std::string path = extractParameter<std::string>(
        convertCreateParametersToString(CreateParameters::FilePath), params);

    if ((Manager::fUserDumpInProgress == true) && (dumpType == DumpTypes::USER))
    {
        lg2::info("Another user initiated dump in progress");
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    lg2::info("Initiating new BMC dump with type: {TYPE} path: {PATH}", "TYPE",
              dumpTypeToString(dumpType).value(), "PATH", path);

    auto id = captureDump(dumpType, path);
    std::filesystem::path objPath;

    try
    {
        uint64_t timeStamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        objPath = createEntry(id, timeStamp, 0, std::string(),
                              phosphor::dump::OperationStatus::InProgress,
                              originatorId, originatorType);
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error("Error in creating dump entry, errormsg: {ERROR}, "
                   "ID: {ID}",
                   "ERROR", e, "ID", id);
        elog<InternalFailure>();
    }

    if (dumpType == DumpTypes::USER)
    {
        Manager::fUserDumpInProgress = true;
    }
    return objPath.string();
}

uint32_t Manager::captureDump(DumpTypes type, const std::string& path)
{
    // Get Dump size.
    auto size = getAllowedSize(dumpDir, BMC_DUMP_MAX_SIZE,
                               BMC_DUMP_MIN_SPACE_REQD, BMC_DUMP_TOTAL_SIZE);

    pid_t pid = fork();

    if (pid == 0)
    {
        std::filesystem::path dumpPath(dumpDir);
        auto id = std::to_string(lastEntryId + 1);
        dumpPath /= id;

        auto strType = dumpTypeToString(type).value();
        execl("/usr/bin/dreport", "dreport", "-d", dumpPath.c_str(), "-i",
              id.c_str(), "-s", std::to_string(size).c_str(), "-q", "-v", "-p",
              path.empty() ? "" : path.c_str(), "-t", strType.c_str(), nullptr);

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
            if (type == DumpTypes::USER)
            {
                lg2::info("User initiated dump completed, resetting flag");
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

void Manager::createEntry(const std::filesystem::path& file)
{
    // Dump File Name format obmcdump_ID_EPOCHTIME.EXT
    static constexpr auto ID_POS = 1;
    static constexpr auto EPOCHTIME_POS = 2;
    std::regex file_regex("obmcdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)");

    std::string filename = file.filename();
    std::smatch match;

    if (!std::regex_search(filename, match, file_regex) || match.size() < 2)
    {
        lg2::error("Invalid Dump file name, FILENAME: {FILENAME}", "FILENAME",
                   file);
        return;
    }

    try
    {
        // Extract ID and timestamp from the filename
        uint64_t id = std::stoul(match[ID_POS]);
        uint64_t timestamp = extractTimestamp(match[EPOCHTIME_POS]);

        // If there is an existing entry update it or create a new one
        createOrUpdateEntry(id, timestamp, std::filesystem::file_size(file),
                            file, phosphor::dump::OperationStatus::Completed,
                            std::string(), OriginatorTypes::Internal);
    }
    catch (const std::exception& e)
    {
        lg2::error("Unable to get id or timestamp from file name, "
                   "FILENAME: {FILENAME} ERROR: {ERROR}",
                   "FILENAME", file, "ERROR", e);
        return;
    }
}

uint64_t Manager::extractTimestamp(const std::string& matchString)
{
    const uint64_t multiplier = 1000000ULL; // To convert to microseconds

    return std::stoull(matchString) * multiplier;
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
            auto fileIt = std::filesystem::directory_iterator(p.path());
            // Create dump entry d-bus object.
            if (fileIt != std::filesystem::end(fileIt))
            {
                createEntry(fileIt->path());
            }
        }
    }
}

void Manager::erase(uint32_t entryId)
{
    lg2::error("> dump manager erase");
    entries.erase(entryId);
    lg2::error("< dump manager erase");
}

void Manager::deleteAll()
{
    auto iter = entries.begin();
    while (iter != entries.end())
    {
        auto& entry = iter->second;
        ++iter;
        entry->delete_();
    }
}

std::filesystem::path Manager::createEntry(
    const uint32_t id, const uint64_t ms, uint64_t fileSize,
    const std::filesystem::path& file, phosphor::dump::OperationStatus status,
    std::string originatorId, OriginatorTypes originatorType)
{
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);
    lastEntryId = std::max(lastEntryId, id);
    try
    {
        entries.emplace(
            id, std::make_unique<phosphor::dump::new_::Entry<
                    sdbusplus::xyz::openbmc_project::Dump::Entry::server::BMC,
                    phosphor::dump::DumpEntryHelper>>(
                    bus, objPath.c_str(), id, ms, fileSize, file, status,
                    originatorId, originatorType, *this));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error(
            "Error in creating dump entry, errormsg: {ERROR}, "
            "OBJECTPATH: {OBJECT_PATH}, ID: {ID}, TIMESTAMP: {TIMESTAMP}, "
            "SIZE: {SIZE}, FILENAME: {FILENAME}",
            "ERROR", e, "OBJECT_PATH", objPath, "ID", id, "TIMESTAMP", ms,
            "SIZE", fileSize, "FILENAME", file);
        throw;
    }
    return objPath;
}

std::filesystem::path Manager::createOrUpdateEntry(
    const uint32_t id, const uint64_t timestamp, uint64_t fileSize,
    const std::filesystem::path& file, phosphor::dump::OperationStatus status,
    std::string originatorId, OriginatorTypes originatorType)
{
    auto dumpEntry = getEntry(id);
    if (dumpEntry != nullptr)
    {
        dumpEntry->markComplete(timestamp, std::filesystem::file_size(file),
                                file);
    }
    else
    {
        return createEntry(id, timestamp, fileSize, file, status, originatorId,
                           originatorType);
    }
    return std::filesystem::path(baseEntryPath) / std::to_string(id);
}
} // namespace bmc
} // namespace dump
} // namespace phosphor
