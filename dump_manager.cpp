#include "dump_manager.hpp"
#include "dump_internal.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "config.h"

#include <phosphor-logging/elog-errors.hpp>
#include <unistd.h>

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

    auto dumpId = captureDump(
                      Type::UserRequested, paths);
    return dumpId;
}

uint32_t Manager:: captureDump(
    Type type,
    const std::vector<std::string>& fullPaths)
{
    pid_t pid = fork();

    // TODO openbmc/openbmc#1795
    // Add Dump location info.
    if (pid == 0)
    {
        execl("/usr/bin/ffdc", "ffdc", (char*)0);

        //ffdc script execution is failed.
        log<level::ERR>("Error occurred during ffdc function execution");
        elog<InternalFailure>();
    }
    else if (pid > 0)
    {
        auto rc = sd_event_add_child(eventLoop,
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
                        entry("ERRNO=%s", strerror(error)));
        elog<InternalFailure>();
    }

    return ++lastEntryId;
}

void Manager::createEntry(const fs::path& file)
{
    // TODO openbmc/openbmc#1795
    // Get Dump ID from Dump file name.
    // Validate the Dump file name.
    auto id = lastEntryId;

    //Get Epoch time.
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch()).count();

    // Entry Object path.
    auto objPath =  std::string(OBJ_ENTRY) + '/' +
                    std::to_string(id);

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
    auto entry = entries.find(entryId);
    if (entries.end() != entry)
    {
        entries.erase(entry);
    }
}

} //namespace dump
} //namespace phosphor
