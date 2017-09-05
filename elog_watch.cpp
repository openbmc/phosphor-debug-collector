#include <phosphor-logging/elog.hpp>

#include "elog_watch.hpp"
#include "dump_internal.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"
#include "dump_serialize.hpp"

namespace phosphor
{
namespace dump
{
namespace elog
{

using namespace phosphor::logging;
constexpr auto LOG_PATH = "/xyz/openbmc_project/logging";
constexpr auto INTERNAL_FAILURE =
    "xyz.openbmc_project.Common.Error.InternalFailure";
using Message = std::string;
using Attributes = sdbusplus::message::variant<Message>;
using AttributeName = std::string;
using AttributeMap = std::map<AttributeName, Attributes>;
using PropertyName = std::string;
using PropertyMap = std::map<PropertyName, AttributeMap>;
using LogEntryMsg = std::pair<sdbusplus::message::object_path, PropertyMap>;

Watch::Watch(sdbusplus::bus::bus& bus, IMgr& iMgr):
    iMgr(iMgr),
    addMatch(
        bus,
        sdbusplus::bus::match::rules::interfacesAdded() +
        sdbusplus::bus::match::rules::path_namespace(
            OBJ_LOGGING),
        std::bind(std::mem_fn(&Watch::addCallback),
                  this, std::placeholders::_1)),
    delMatch(
        bus,
        sdbusplus::bus::match::rules::interfacesRemoved() +
        sdbusplus::bus::match::rules::path_namespace(
            OBJ_LOGGING),
        std::bind(std::mem_fn(&Watch::delCallback),
                  this, std::placeholders::_1))
{

    fs::path file(ELOG_ID_PERSIST_PATH);
    if (fs::exists(file))
    {
        if (!deserialize(ELOG_ID_PERSIST_PATH, elogList))
        {
            log<level::ERR>("Error occurred during error id deserialize");
        }
    }
}
void Watch::addCallback(sdbusplus::message::message& msg)
{
    using Type =
        sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;
    using QuotaExceeded =
        sdbusplus::xyz::openbmc_project::Dump::Create::Error::QuotaExceeded;

    LogEntryMsg logEntry;
    msg.read(logEntry);

    std::string objectPath(std::move(logEntry.first));

    std::size_t found = objectPath.find("entry");
    if (found == std::string::npos)
    {
        //Not a new error entry skip
        return;
    }

    auto eId = getEid(objectPath);

    auto search = elogList.find(eId);
    if (search != elogList.end())
    {
        //elog exists in the list, Skip the dump
        return;
    }

    auto iter = logEntry.second.find("xyz.openbmc_project.Logging.Entry");
    if (iter == logEntry.second.end())
    {
        return;
    }

    auto attr = iter->second.find("Message");
    if (attr == iter->second.end())
    {
        return;
    }

    auto& data =
        sdbusplus::message::variant_ns::get<PropertyName>(attr->second);
    if (data.empty())
    {
        //No Message skip
        return;
    }

    if (data != INTERNAL_FAILURE)
    {
        //Not a InternalFailure, skip
        return;
    }

    std::vector<std::string> fullPaths;
    fullPaths.push_back(objectPath);

    try
    {
        //Save the elog information. This is to avoid dump requests
        //in elog restore path.
        elogList.insert(eId);

        serialize(elogList);

        //Call internal create function to initiate dump
        iMgr.IMgr::create(Type::InternalFailure, fullPaths);
    }
    catch (QuotaExceeded& e)
    {
        //No action now
    }
    return;
}

void Watch::delCallback(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path logEntry;
    msg.read(logEntry);

    //Get elog entry message string.
    std::string objectPath(logEntry);

    //Get elog id
    auto eId = getEid(objectPath);

    //Delete the elog entry from the list and serialize
    auto search = elogList.find(eId);
    if (search != elogList.end())
    {
        elogList.erase(search);
        serialize(elogList);
    }
}

EId Watch::getEid(const std::string& objectPath)
{
    fs::path path(objectPath);
    return std::stoul(path.filename());
}

}//namespace elog
}//namespace dump
}//namespace phosphor
