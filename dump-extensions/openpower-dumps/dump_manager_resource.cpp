#include "config.h"

#include "dump_manager_resource.hpp"

#include "dump_utils.hpp"
#include "resource_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

namespace openpower
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

    // If there is an entry with this sourceId or an invalid id
    // update that.
    // If host is sending the source id before the completion
    // the source id will be updated by the transport layer with host.
    // if not the source id will stay as invalid one.
    for (auto& entry : entries)
    {
        openpower::dump::resource::Entry* resEntry =
            dynamic_cast<openpower::dump::resource::Entry*>(entry.second.get());
        if ((resEntry->status() ==
             phosphor::dump::OperationStatus::InProgress) &&
            ((resEntry->sourceDumpId() == dumpId) ||
             (resEntry->sourceDumpId() == INVALID_SOURCE_ID)))
        {
            resEntry->update(timeStamp, size, dumpId);
            return;
        }
    }
    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;

    try
    {
        entries.insert(std::make_pair(
            id, std::make_unique<resource::Entry>(
                    bus, objPath.c_str(), id, timeStamp, size, dumpId,
                    std::string(), std::string(),
                    phosphor::dump::OperationStatus::Completed, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(e.what());
        log<level::ERR>("Error in creating resource dump entry",
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

    // Allow creating resource dump only when the host is up.
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
    auto objPath = std::filesystem::path(baseEntryPath) / idString;
    std::time_t timeStamp = std::time(nullptr);

    std::string vspString = params[sdbusplus::com::ibm::Dump::server::Create::
                                       convertCreateParametersToString(
                                           CreateParameters::VSPString)];
    std::string pwd =
        params[sdbusplus::com::ibm::Dump::server::Create::
                   convertCreateParametersToString(CreateParameters::Password)];

    try
    {
        entries.insert(std::make_pair(
            id, std::make_unique<resource::Entry>(
                    bus, objPath.c_str(), id, timeStamp, 0, INVALID_SOURCE_ID,
                    vspString, pwd, phosphor::dump::OperationStatus::InProgress,
                    *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(e.what());
        log<level::ERR>("Error in creating resource dump entry",
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
} // namespace openpower
