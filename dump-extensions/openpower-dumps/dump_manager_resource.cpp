#include "config.h"

#include "dump_manager_resource.hpp"

#include "resource_dump_entry.hpp"

#include <phosphor-logging/elog.hpp>

namespace phosphor
{
namespace dump
{
namespace resource
{

using namespace phosphor::logging;

void Manager::notify(uint32_t dumpId, uint64_t size)
{

    // Get the timestamp
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();

    // If there is an entry with this  sourceId update that.
    for (auto& entry : entries)
    {
        phosphor::dump::resource::Entry* resEntry =
            dynamic_cast<phosphor::dump::resource::Entry*>(entry.second.get());
        if ((resEntry->status() ==
             phosphor::dump::OperationStatus::InProgress) &&
            (resEntry->sourceDumpId() == dumpId))
        {
            resEntry->update(ms, size);
            return;
        }
    }
    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = fs::path(baseEntryPath) / idString;
    entries.insert(std::make_pair(
        id,
        std::make_unique<resource::Entry>(
            bus, objPath.c_str(), id, ms, size, dumpId, std::string(),
            std::string(), phosphor::dump::OperationStatus::Completed, *this)));
    lastEntryId++;
}

sdbusplus::message::object_path
    Manager::createDump(std::map<std::string, std::string> params)
{
    using CreateParameters =
        sdbusplus::com::ibm::Dump::server::Create::CreateParameters;
    auto id = ++lastEntryId;
    auto idString = std::to_string(id);
    auto objPath = fs::path(baseEntryPath) / idString;
    std::string vspString = params[sdbusplus::com::ibm::Dump::server::Create::
                                       convertCreateParametersToString(
                                           CreateParameters::VSPString)];
    std::string pwd =
        params[sdbusplus::com::ibm::Dump::server::Create::
                   convertCreateParametersToString(CreateParameters::Password)];
    entries.insert(std::make_pair(
        id, std::make_unique<resource::Entry>(
                bus, objPath.c_str(), id, 0, 0, INVALID_SOURCE_ID,
                std::string(), std::string(),
                phosphor::dump::OperationStatus::InProgress, *this)));
    return std::string(objPath.c_str());
}

} // namespace resource
} // namespace dump
} // namespace phosphor
