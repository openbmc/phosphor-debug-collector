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
        execl("/usr/bin/ffdc", "ffdc", "-d", dumpPath.c_str(), "-e", nullptr);

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
        // For any new dump file create dump entry object.
        if (IN_CLOSE_WRITE == i.second)
        {
            createEntry(i.first);
        }
    }
}

void Manager::restore()
{
    std::vector<uint32_t> dumpIds;

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
            dumpIds.push_back(std::stoul(idStr));

            auto fileIt = fs::directory_iterator(p.path());
            //Create dump entry d-bus object.
            if (fileIt != end(fileIt))
            {
                createEntry(fileIt->path());
            }
        }
    }

    if (!dumpIds.empty())
    {
        lastEntryId = *(std::max_element(dumpIds.begin(), dumpIds.end()));
    }
}

} //namespace dump
} //namespace phosphor
