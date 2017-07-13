#include <unistd.h>
#include <sys/inotify.h>

#include <phosphor-logging/elog-errors.hpp>

#include "dump_manager.hpp"
#include "dump_internal.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
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
    // TODO openbmc/openbmc#1795
    // Add implementaion of internal create function.
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
    //Increment Dump ID
    lastEntryId++;

    pid_t pid = fork();

    if (pid == 0)
    {
        fs::path dumpPath(BMC_DUMP_PATH);

        dumpPath /= std::to_string(lastEntryId);
        execl("/usr/bin/ffdc", "ffdc", "-d", dumpPath.c_str(), nullptr);

        //ffdc script execution is failed.
        auto error = errno;
        log<level::ERR>("Error occurred during ffdc function execution",
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

    return lastEntryId;
}

void Manager::createEntry(const fs::path& file)
{
    // TODO openbmc/openbmc#1795
    // Get Dump ID and Epoch time from Dump file name.
    // Validate the Dump file name.
    auto id = lastEntryId;

    //Get Epoch time.
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch()).count();

    // Entry Object path.
    auto objPath =  fs::path(OBJ_ENTRY) / std::to_string(id);

    auto size = fs::file_size(file);

    entries.insert(std::make_pair(id,
                                  std::make_unique<Entry>(
                                      bus,
                                      objPath.c_str(),
                                      id,
                                      ms,
                                      size,
                                      file,
                                      *this)));
}

void Manager::erase(uint32_t entryId)
{
    entries.erase(entryId);
}

void Manager::watchCallback(UserMap fileInfo)
{
    for (const auto& i : fileInfo)
    {
        // For any new dump file create dump entry object.
        if (IN_CLOSE_WRITE == i.second)
        {
            createEntry(i.first);
        }
    }
}

} //namespace dump
} //namespace phosphor
