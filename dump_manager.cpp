#include <unistd.h>
#include <sys/inotify.h>
#include <regex>

#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>

#include "dump_manager.hpp"
#include "dump_internal.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"
#include "config.h"

namespace phosphor
{
namespace dump
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

namespace internal
{

void Manager::create(
    Type type,
    std::vector<std::string> fullPaths)
{
    dumpMgr.phosphor::dump::Manager::captureDump(type, fullPaths);
}

} //namepsace internal

uint32_t Manager::createDump()
{
    std::vector<std::string> paths;

    return captureDump(Type::UserRequested, paths);
}

uint32_t Manager::captureDump(
    Type type,
    const std::vector<std::string>& fullPaths)
{
    using namespace sdbusplus::xyz::openbmc_project::Dump::Create::Error;
    using Reason = xyz::openbmc_project::Dump::Create::QuotaExceeded::REASON;

    //Get Dump size.
    auto size = getAllowedSize();
    if (size == 0)
    {
        log<level::ERR>("No enough space: Delete old dumps");
        elog<QuotaExceeded>(Reason("No enough space: Delete old dumps"));
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        fs::path dumpPath(BMC_DUMP_PATH);
        auto tempId =  std::to_string(lastEntryId + 1);
        dumpPath /= tempId;

        //Get Dump type enum.
        auto tempType =
            std::to_string(static_cast<std::underlying_type<Type>::type>(type));

         execl("/usr/bin/dreport",
              "dreport",
              "-d", dumpPath.c_str(),
              "-i", tempId.data(),
              "-s", std::to_string(size).data(),
              "-q",
              "-f", fullPaths.front(),
              "-t", tempType.data(),
              nullptr);

        //dreport script execution is failed.
        auto error = errno;
        log<level::ERR>("Error occurred during dreport function execution",
                        entry("ERRNO=%d", error));
        elog<InternalFailure>();
    }
    else if (pid > 0)
    {
        auto rc = sd_event_add_child(eventLoop.get(),
                                     nullptr,
                                     pid,
                                     WEXITED | WSTOPPED,
                                     callback,
                                     nullptr);
        if (0 > rc)
        {
            // Failed to add to event loop
            log<level::ERR>("Error occurred during the sd_event_add_child call",
                            entry("rc=%d", rc));
            elog<InternalFailure>();
        }
    }
    else
    {
        auto error = errno;
        log<level::ERR>("Error occurred during fork",
                        entry("ERRNO=%d", error));
        elog<InternalFailure>();
    }

    //Increment dump in proress counter.
    progressCounter++;

    return ++lastEntryId;
}

void Manager::createEntry(const fs::path& file)
{
    //Decrement the Dump in progress counter.
    progressCounter = (progressCounter == 0 ? 0 : progressCounter - 1);

    //Dump File Name format obmcdump_ID_EPOCHTIME.EXT
    static constexpr auto ID_POS         = 1;
    static constexpr auto EPOCHTIME_POS  = 2;
    std::regex file_regex("obmcdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)");

    std::smatch match;
    std::string name = file.filename();

    if (!((std::regex_search(name, match, file_regex)) &&
          (match.size() > 0)))
    {
        log<level::ERR>("Invalid Dump file name",
                        entry("Filename=%s", file.filename()));
        return;
    }

    auto idString = match[ID_POS];
    auto msString = match[EPOCHTIME_POS];

    try
    {
        auto id = stoul(idString);
        // Entry Object path.
        auto objPath =  fs::path(OBJ_ENTRY) / std::to_string(id);

        entries.insert(std::make_pair(id,
                                      std::make_unique<Entry>(
                                          bus,
                                          objPath.c_str(),
                                          id,
                                          stoull(msString),
                                          fs::file_size(file),
                                          file,
                                          *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(e.what());
        return;
    }
}

void Manager::erase(uint32_t entryId)
{
    entries.erase(entryId);
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
        else if ((IN_CREATE == i.second) && fs::is_directory(i.first))
        {
            auto watchObj = std::make_unique<Watch>(
                                eventLoop,
                                IN_NONBLOCK,
                                IN_CLOSE_WRITE,
                                EPOLLIN,
                                i.first,
                                std::bind(
                                    std::mem_fn(
                                       &phosphor::dump::Manager::watchCallback),
                                    this, std::placeholders::_1));

            childWatchMap.emplace(i.first, std::move(watchObj));
        }

    }
}

void Manager::removeWatch(const fs::path& path)
{
    //Delete Watch entry from map.
    childWatchMap.erase(path);
}

void Manager::restore()
{
    fs::path dir(BMC_DUMP_PATH);
    if (!fs::exists(dir) || fs::is_empty(dir))
    {
        return;
    }

    //Dump file path: <BMC_DUMP_PATH>/<id>/<filename>
    for (const auto& p : fs::directory_iterator(dir))
    {
        auto idStr = p.path().filename().string();

        //Consider only directory's with dump id as name.
        //Note: As per design one file per directory.
        if ((fs::is_directory(p.path())) &&
            std::all_of(idStr.begin(), idStr.end(), ::isdigit))
        {
            lastEntryId = std::max(lastEntryId,
                                   static_cast<uint32_t>(std::stoul(idStr)));
            auto fileIt = fs::directory_iterator(p.path());
            //Create dump entry d-bus object.
            if (fileIt != fs::end(fileIt))
            {
                createEntry(fileIt->path());
            }
        }
    }
}

uint32_t Manager::getAllowedSize()
{
    // setting minimum dump size to 10K. This will help
    // to collect minimum dump data incase dump is close
    // maximium szie.
    static constexpr auto  BMC_DUMP_MINI_SIZE = 10;

    auto size = 0;

    // Maximum number of dump is based on total dump partion size
    // and individual dump Max size configured in the system.
    // Existing dump entry is calculated based on created dump entries
    // plus in progress dumps.

    if ((entries.size() + progressCounter) <
        (BMC_DUMP_TOTAL_SIZE / BMC_DUMP_SIZE))
    {
        return BMC_DUMP_SIZE;
    }

    //Get current size of the dump directory.
    for (const auto& p : fs::recursive_directory_iterator(BMC_DUMP_PATH))
    {
        if (!fs::is_directory(p))
        {
            size += fs::file_size(p);
        }
    }

    //Convert size into KB
    size = size / 1024;

    size = (size > BMC_DUMP_TOTAL_SIZE ? 0 : BMC_DUMP_TOTAL_SIZE - size);

    if (size > BMC_DUMP_MINI_SIZE)
    {
        size = (size > BMC_DUMP_SIZE ? BMC_DUMP_SIZE :  BMC_DUMP_MINI_SIZE);
    }
    else
    {
        size = 0;
    }
    return size;
}

} //namespace dump
} //namespace phosphor
