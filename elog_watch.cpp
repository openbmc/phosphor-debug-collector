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

void Watch::callback(sdbusplus::message::message& msg)
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

    if (INTERNAL_FAILURE != data)
    {
        //Not a InternalFailure, skip
        return;
    }

    std::vector<std::string> fullPaths;
    fullPaths.push_back(objectPath);

    try
    {
        //Call internal create function to initiate dump
        iMgr.IMgr::create(Type::InternalFailure, fullPaths);
    }
    catch (QuotaExceeded& e)
    {
        //No action now
    }
    return;
}

}//namespace elog
}//namespace dump
}//namespace phosphor
