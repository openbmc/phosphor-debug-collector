#include <unistd.h>
#include <sys/inotify.h>
#include <regex>

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
using namespace std;

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
    pid_t pid = fork();

    if (pid == 0)
    {
        fs::path dumpPath(BMC_DUMP_PATH);

        dumpPath /= std::to_string(lastEntryId + 1);
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

    return ++lastEntryId;
}

void Manager::createEntry(const fs::path& file)
{
    //Dump File Name format obmcdump_ID_EPOCHTIME.EXT
    static constexpr auto ID_POS         = 1;
    static constexpr auto EPOCHTIME_POS  = 2;

    //Extract Dump ID from file name.
    auto idString = getToken(file.filename(), ID_POS);

    if (idString.empty())
    {
        log<level::ERR>("Invalid id info in the Dump file",
                        entry("Filename=%s", file.filename()));
        return;
    }

    //Extract Epoch time from file name.
    auto ms = getToken(file.filename(), EPOCHTIME_POS);

    if (ms.empty())
    {
        log<level::ERR>("Invalid epochtime info in the dump file",
                        entry("Filename=%s", file.filename()));
        return;
    }

    auto id = stoi(idString.c_str());

    // Entry Object path.
    auto objPath =  fs::path(OBJ_ENTRY) / std::to_string(id);

    auto size = fs::file_size(file);

    entries.insert(std::make_pair(id,
                                  std::make_unique<Entry>(
                                      bus,
                                      objPath.c_str(),
                                      id,
                                      stoi(ms.c_str()),
                                      size,
                                      file,
                                      *this)));
}

void Manager::erase(uint32_t entryId)
{
    entries.erase(entryId);
}

void Manager::watchCallback(const UserMap& fileInfo)
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

std::string Manager::getToken(const std::string& name, const uint32_t pos)
{
    std::string s = name;

    //returns empty string if operations fails.
    auto token = std::string("");

    try
    {
        //String name format obmcdump_[ID]_[epochtime].[EXT]
        std::regex e("obmcdump_([0-9^_]+)_([0-9^_.]+)");
        std::smatch match;

        if ((std::regex_search(s, match, e)) &&
            (match.size() > pos) &&
            (pos != 0))
        {
            token =  match.str(pos);
        }
    }
    catch (std::regex_error& e)
    {
        log<level::ERR>(e.what());
    }
    return token;
}

} //namespace dump
} //namespace phosphor
