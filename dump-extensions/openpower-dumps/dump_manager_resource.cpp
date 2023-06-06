#include "config.h"

#include "dump_manager_resource.hpp"

#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "resource_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

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
    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // If there is an entry with invalid id update that.
    // If there a completed one with same source id ignore it
    // if there is no invalid id, create new entry
    openpower::dump::resource::Entry* upEntry = NULL;
    for (auto& entry : entries)
    {
        openpower::dump::resource::Entry* resEntry =
            dynamic_cast<openpower::dump::resource::Entry*>(entry.second.get());

        // If there is already a completed entry with input source id then
        // ignore this notification.
        if ((resEntry->sourceDumpId() == dumpId) &&
            (resEntry->status() == phosphor::dump::OperationStatus::Completed))
        {
            lg2::info(
                "Resource dump entry with source dump id: {DUMP_ID} "
		"is already present with entry id: {ENTRY_ID}",
                "DUMP_ID", dumpId, "ENTRY_ID", resEntry->getDumpId());
            return;
        }

        // Save the first entry with INVALID_SOURCE_ID
        // but continue in the loop to make sure the
        // new entry is not duplicate
        if ((resEntry->status() ==
             phosphor::dump::OperationStatus::InProgress) &&
            (resEntry->sourceDumpId() == INVALID_SOURCE_ID) &&
            (upEntry == NULL))
        {
            upEntry = resEntry;
        }
    }
    if (upEntry != NULL)
    {
        lg2::info(
            "Resource Dump Notify: Updating dumpId: {DUMP_ID} with "
	    "source Id: {SOURCE_ID} Size: {SIZE}",
            "DUMP_ID", upEntry->getDumpId(), "SOURCE_ID", dumpId, "SIZE", size);
        upEntry->update(timeStamp, size, dumpId);
        return;
    }

    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;

    // TODO: Get the originator Id, type from the persisted file.
    // For now replacing it with null

    try
    {
        lg2::info(
            "Resouce Dump Notify: creating new dump entry dumpId: {DUMP_ID} "
	    "Id: {ID} Size: {SIZE}",
            "DUMP_ID", id, "ID", dumpId, "SIZE", size);
        entries.insert(std::make_pair(
            id, std::make_unique<resource::Entry>(
                    bus, objPath.c_str(), id, timeStamp, size, dumpId,
                    std::string(), std::string(),
                    phosphor::dump::OperationStatus::Completed, std::string(),
                    originatorTypes::Internal, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error(
            "Error in creating resource dump entry, errormsg: {ERROR_MSG}, "
	    "OBJECTPATH: {OBJECT_PATH}, ID: {ID}, TIMESTAMP: {TIMESTAMP}, "
	    "SIZE: {SIZE}, SOURCEID: {SOURCE_ID}",
            "ERROR_MSG", e, "OBJECT_PATH", objPath.c_str(), "ID", id,
            "TIMESTAMP", timeStamp, "SIZE", size, "SOURCE_ID", dumpId);
        report<InternalFailure>();
        return;
    }
    lastEntryId++;
}

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
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

    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;
    using CreateParameters =
        sdbusplus::com::ibm::Dump::server::Create::CreateParameters;

    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;
    uint64_t timeStamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    std::string vspString;
    auto iter = params.find(
        sdbusplus::com::ibm::Dump::server::Create::
            convertCreateParametersToString(CreateParameters::VSPString));
    if (iter == params.end())
    {
        // Host will generate a default dump if no resource selector string
        // is provided. The default dump will be a non-disruptive system dump.
        lg2::info(
            "VSP string is not provided, a non-disruptive system dump will be "
            "generated by the host");
    }
    else
    {
        try
        {
            vspString = std::get<std::string>(iter->second);
        }
        catch (const std::bad_variant_access& e)
        {
            // Exception will be raised if the input is not string
            lg2::error("An invalid vsp string is passed, errormsg: {ERROR_MSG}",
                       "ERROR_MSG", e);
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("VSP_STRING"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }

    std::string pwd;
    iter = params.find(
        sdbusplus::com::ibm::Dump::server::Create::
            convertCreateParametersToString(CreateParameters::Password));
    if (iter == params.end())
    {
        lg2::info("Password is not provided for resource dump");
    }
    else
    {
        try
        {
            pwd = std::get<std::string>(iter->second);
        }
        catch (const std::bad_variant_access& e)
        {
            // Exception will be raised if the input is not string
            lg2::error(
                "An invalid password string is passed, errormsg: {ERROR_MSG}",
                "ERROR_MSG", e);
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("PASSWORD"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }

    // Get the originator id and type from params
    std::string originatorId;
    originatorTypes originatorType;

    phosphor::dump::extractOriginatorProperties(params, originatorId,
                                                originatorType);

    try
    {
        entries.insert(std::make_pair(
            id, std::make_unique<resource::Entry>(
                    bus, objPath.c_str(), id, timeStamp, 0, INVALID_SOURCE_ID,
                    vspString, pwd, phosphor::dump::OperationStatus::InProgress,
                    originatorId, originatorType, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error(
            "Error in creating resource dump entry, errormsg: {ERROR_MSG}, "
	    "OBJECTPATH: {OBJECT_PATH}, VSPSTRING: {VSP_STRING}, ID: {ID}",
            "ERROR_MSG", e, "OBJECT_PATH", objPath.c_str(), "VSP_STRING",
            vspString, "ID", id);
        elog<InternalFailure>();
        return std::string();
    }
    lastEntryId++;
    return objPath.string();
}

} // namespace resource
} // namespace dump
} // namespace openpower
