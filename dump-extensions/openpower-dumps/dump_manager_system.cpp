#include "config.h"

#include "dump_manager_system.hpp"

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_utils.hpp"
#include "op_dump_util.hpp"
#include "system_dump_entry.hpp"
#include "system_dump_serialize.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

namespace openpower
{
namespace dump
{
namespace system
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

void Manager::notify(uint32_t dumpId, uint64_t size)
{

    // Get the timestamp
    std::time_t timeStamp = std::time(nullptr);

    // System dump can get created due to a fault in server
    // or by request from user. A system dump by fault is
    // first reported here, but for a user requested dump an
    // entry will be created first with invalid source id.
    // Since there can be only one system dump creation at a time,
    // if there is an entry with invalid sourceId update that.
    for (auto& entry : entries)
    {
        openpower::dump::system::Entry* sysEntry =
            dynamic_cast<openpower::dump::system::Entry*>(entry.second.get());
        if (sysEntry->sourceDumpId() == INVALID_SOURCE_ID)
        {
            sysEntry->update(timeStamp, size, dumpId);
            return;
        }
    }

    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;

    // TODO: Get the generator Id from the persisted file.
    // For now replacing it with null
    try
    {
        auto entry = std::make_unique<system::Entry>(
            bus, objPath.c_str(), id, timeStamp, size, dumpId, std::string(),
            phosphor::dump::OperationStatus::Completed, baseEntryPath, *this);
        serialize(*entry.get());
        entries.insert(std::make_pair(id, std::move(entry)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(
            fmt::format(
                "Error in creating system dump entry, errormsg({}), "
                "OBJECTPATH({}), ID({}), TIMESTAMP({}),SIZE({}), SOURCEID({})",
                e.what(), objPath.c_str(), id, timeStamp, size, dumpId)
                .c_str());
        report<InternalFailure>();
        return;
    }
    lastEntryId++;
    return;
}

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
    constexpr auto DIAG_MOD_TARGET = "obmc-host-crash@0.target";

    if (params.size() > 1)
    {
        log<level::WARNING>(
            "System dump accepts not more than 1 additional parameter");
    }

    // Check dump policy
    util::isOPDumpsEnabled();

    using NotAllowed =
        sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
    using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

    // Allow creating system dump only when the host is up.
    if (!phosphor::dump::isHostRunning())
    {
        elog<NotAllowed>(
            Reason("System dump can be initiated only when the host is up"));
        return std::string();
    }

    // Get the generator id from params
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;
    using CreateParameters =
        sdbusplus::xyz::openbmc_project::Dump::server::Create::CreateParameters;

    std::string generatorId;
    auto iter = params.find(
        sdbusplus::xyz::openbmc_project::Dump::server::Create::
            convertCreateParametersToString(CreateParameters::GeneratorId));
    if (iter == params.end())
    {
        log<level::INFO>(
            "GeneratorId is not provided. Replacing the string with null");
    }
    else
    {
        try
        {
            generatorId = std::get<std::string>(iter->second);
        }
        catch (const std::bad_variant_access& e)
        {
            // Exception will be raised if the input is not string
            log<level::ERR>(
                "An invalid  generatorId passed. It should be a string",
                entry("ERROR_MSG=%s", e.what()));
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("GENERATOR_ID"),
                                  Argument::ARGUMENT_VALUE("INVALID INPUT"));
        }
    }

    auto b = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append(DIAG_MOD_TARGET); // unit to activate
    method.append("replace");
    bus.call_noreply(method);

    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = std::filesystem::path(baseEntryPath) / idString;
    std::time_t timeStamp = std::time(nullptr);

    try
    {
        auto entry = std::make_unique<system::Entry>(
            bus, objPath.c_str(), id, timeStamp, 0, INVALID_SOURCE_ID,
            generatorId, phosphor::dump::OperationStatus::InProgress,
            baseEntryPath, *this);
        serialize(*entry.get());
        entries.insert(std::make_pair(id, std::move(entry)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(
            fmt::format("Error in creating system dump entry, errormsg({}), "
                        "OBJECTPATH({}), ID({})",
                        e.what(), objPath.c_str(), id)
                .c_str());
        elog<InternalFailure>();
        return std::string();
    }
    lastEntryId++;
    return objPath.string();
}

void Manager::restore()
{
    std::filesystem::path dir(SYSTEM_DUMP_SERIAL_PATH);
    if (!std::filesystem::exists(dir) || std::filesystem::is_empty(dir))
    {
        return;
    }

    std::vector<uint32_t> dumpIds;
    for (auto& file : std::filesystem::directory_iterator(dir))
    {
        auto idNum = std::stol(file.path().filename().c_str());
        auto idString = std::to_string(idNum);
        auto objPath = std::filesystem::path(baseEntryPath) / idString;
        auto entry =
            std::make_unique<Entry>(bus, objPath, baseEntryPath, *this);
        if (deserialize(file.path(), *entry))
        {
            entries.insert(std::make_pair(idNum, std::move(entry)));
            dumpIds.push_back(idNum);
        }
        else
        {
            log<level::WARNING>("Unable to restore dump entry");
        }
    }
    if (!dumpIds.empty())
    {
        lastEntryId = *(std::max_element(dumpIds.begin(), dumpIds.end()));
    }
}

} // namespace system
} // namespace dump
} // namespace openpower
