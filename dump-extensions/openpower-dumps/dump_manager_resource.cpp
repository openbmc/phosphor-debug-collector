#include "config.h"

#include "dump_manager_resource.hpp"
#include "dump_utils.hpp"

#include "resource_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

namespace phosphor
{
namespace dump
{
namespace resource
{

using namespace phosphor::logging;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

void Manager::notify(uint32_t dumpId, uint64_t size)
{
    // Get the timestamp
    std::time_t timeStamp = std::time(nullptr);

    // If there is an entry with this sourceId update that.
    for (auto& entry : entries)
    {
        phosphor::dump::resource::Entry* resEntry =
            dynamic_cast<phosphor::dump::resource::Entry*>(entry.second.get());
        if ((resEntry->status() ==
             phosphor::dump::OperationStatus::InProgress) &&
            (resEntry->sourceDumpId() == dumpId))
        {
            resEntry->update(timeStamp, size);
            return;
        }
    }
    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = fs::path(baseEntryPath) / idString;

    try
    {
        entries.insert(std::make_pair(
            id,
            std::make_unique<resource::Entry>(
                bus, objPath.c_str(), id, timeStamp, size, dumpId,
                std::string(), std::string(),
                phosphor::dump::OperationStatus::Completed, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(e.what());
        log<level::ERR>("Error in creating system dump entry",
                        entry("OBJECTPATH=%s", objPath.c_str()),
                        entry("ID=%d", id), entry("TIMESTAMP=%ull", timeStamp),
                        entry("SIZE=%d", size), entry("SOURCEID=%d", dumpId));
        report<InternalFailure>();
        return;
    }
    lastEntryId++;
}

sdbusplus::message::object_path
    Manager::createDump(std::map<std::string, std::string> params)
{

    using NotAllowed =
        sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
    using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

    // Allow creating system dump only when the host is up.
    if (!phosphor::dump::isHostRunning())
    {
        elog<NotAllowed>(
            Reason("Resource dump can be initiated only when the host is up"));
        return std::string();
    }
    using CreateParameters =
        sdbusplus::com::ibm::Dump::server::Create::CreateParameters;

    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = fs::path(baseEntryPath) / idString;

    std::string vspString = params[sdbusplus::com::ibm::Dump::server::Create::
                                       convertCreateParametersToString(
                                           CreateParameters::VSPString)];
    std::string pwd =
        params[sdbusplus::com::ibm::Dump::server::Create::
                   convertCreateParametersToString(CreateParameters::Password)];

    try
    {
        entries.insert(std::make_pair(
            id,
            std::make_unique<resource::Entry>(
                bus, objPath.c_str(), id, 0, 0, INVALID_SOURCE_ID, vspString,
                pwd, phosphor::dump::OperationStatus::InProgress, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(e.what());
        log<level::ERR>("Error in creating system dump entry",
                        entry("OBJECTPATH=%s", objPath.c_str()),
                        entry("VSPSTRING=%s", vspString.c_str()),
                        entry("ID=%d", id));
        elog<InternalFailure>();
        return std::string();
    }
    lastEntryId++;
    return objPath.string();
}

} // namespace resource
} // namespace dump
} // namespace phosphor
