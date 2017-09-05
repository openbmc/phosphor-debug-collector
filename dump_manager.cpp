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
    auto id = dumpMgr.phosphor::dump::Manager::captureDump(type, fullPaths);

    //elog entry object association is needed for InternalFailure type dump
    if (Type::InternalFailure == type)
    {
        dumpMgr.phosphor::dump::Manager::assocMap.emplace(id, fullPaths.front());
    }
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
    //Type to dreport type  string map
    static const std::map<Type, std::string> typeMap =
    {
        {Type::ApplicationCored, "core"},
        {Type::UserRequested, "user"},
        {Type::InternalFailure, "elog"}
    };

    //Get Dump size.
    auto size = getAllowedSize();

    pid_t pid = fork();

    if (pid == 0)
    {
        fs::path dumpPath(BMC_DUMP_PATH);
        auto id =  std::to_string(lastEntryId + 1);
        dumpPath /= id;

        //get dreport type map entry
        auto tempType = typeMap.find(type);

        execl("/usr/bin/dreport",
              "dreport",
              "-d", dumpPath.c_str(),
              "-i", id.c_str(),
              "-s", std::to_string(size).c_str(),
              "-q",
              "-v",
              "-P", fullPaths.empty() ? "" : fullPaths.front().c_str(),
              "-t", tempType->second.c_str(),
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
        //Set Elog Association
        setElogAssociation(id);
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

size_t Manager::getAllowedSize()
{
    using namespace sdbusplus::xyz::openbmc_project::Dump::Create::Error;
    using Reason = xyz::openbmc_project::Dump::Create::QuotaExceeded::REASON;

    auto size = 0;

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

    //Set the Dump size to Maximum  if the free space is greater than
    //Dump max size otherwise return the available size.

    size = (size > BMC_DUMP_TOTAL_SIZE ? 0 : BMC_DUMP_TOTAL_SIZE - size);

    if (size < BMC_DUMP_MIN_SPACE_REQD)
    {
        //Reached to maximum limit
        elog<QuotaExceeded>(Reason("Not enough space: Delete old dumps"));
    }
    if (size > BMC_DUMP_MAX_SIZE)
    {
        size = BMC_DUMP_MAX_SIZE;
    }

    return size;
}

void  Manager::setElogAssociation(const uint32_t id)
{
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
    constexpr auto ASSOC_IFACE     = "org.openbmc.Associations";
    constexpr auto ASSOC_PROP     = "associations";
    constexpr auto PROP_IFACE = "org.freedesktop.DBus.Properties";

    using AssociationList =
        std::vector<std::tuple<std::string, std::string, std::string>>;

    auto iter = assocMap.find(id);
    if (iter == assocMap.end())
    {
        //Skip , no association entry found.
        return;
    }

    auto elogPath = iter->second;

    //Delete the entry from map.
    assocMap.erase(id);

    //Initialize dump object path.
    fs::path dumpPath(DUMP_OBJPATH);
    dumpPath /= std::to_string(id);

    auto bus = sdbusplus::bus::new_default();
    auto mapper = bus.new_method_call(
                      MAPPER_BUSNAME,
                      MAPPER_PATH,
                      MAPPER_INTERFACE,
                      "GetObject");
    mapper.append(elogPath.c_str(), std::vector<std::string>({ASSOC_IFACE}));
    auto mapperResponseMsg = bus.call(mapper);
    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call");
        return;
    }
    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);
    if (mapperResponse.empty())
    {
        log<level::ERR>("Error reading mapper response");
        return;
    }

    AssociationList list;
    list.emplace_back(std::make_tuple("DUMP_FWD_ASSOCIATION",
                                      "DUMP_REV_ASSOCIATION",
                                      dumpPath.c_str()));
    const auto& host = mapperResponse.cbegin()->first;
    auto m = bus.new_method_call(
                 host.c_str(),
                 elogPath.c_str(),
                 PROP_IFACE,
                 "Set");
    m.append(ASSOC_IFACE);
    m.append(ASSOC_PROP);
    m.append(list);
    bus.call_noreply(m);
}

} //namespace dump
} //namespace phosphor
