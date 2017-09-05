#include <phosphor-logging/elog.hpp>

#include "elog_watch.hpp"
#include "dump_internal.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

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

void Watch::addCallback(sdbusplus::message::message& msg)
{
    using Type =
        sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;
    using QuotaExceeded =
        sdbusplus::xyz::openbmc_project::Dump::Create::Error::QuotaExceeded;

    LogEntryMsg logEntry;
    msg.read(logEntry);

    std::string objectPath(std::move(logEntry.first));

    if (std::find(elogList.begin(), elogList.end(), objectPath)
        != elogList.end())
    {
        //elog exists in the list, Skip the dump
        return;
    }
    std::size_t found = objectPath.find("entry");
    if (found == std::string::npos)
    {
        //Not a new error entry skip
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

    if (strcmp(INTERNAL_FAILURE, data.c_str()))
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
        elogList.emplace_back(objectPath);

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
    std::string objectPath(logEntry);

    //Delete the elog entry from the list.
    std::remove(elogList.begin(), elogList.end(), objectPath);
}

}//namespace elog
}//namespace dump
}//namespace phosphor
