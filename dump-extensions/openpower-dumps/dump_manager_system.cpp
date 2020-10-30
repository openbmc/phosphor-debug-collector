#include "config.h"

#include "dump_manager_system.hpp"

#include "system_dump_entry.hpp"

#include <phosphor-logging/elog.hpp>

namespace phosphor
{
namespace dump
{
namespace system
{

using namespace phosphor::logging;

void Manager::notify(uint32_t dumpId, uint64_t size)
{

    // Get the timestamp
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();

    // If there is an entry with invalid sourceId update that.
    for (auto& entry : entries)
    {
        phosphor::dump::system::Entry* sysEntry =
            dynamic_cast<phosphor::dump::system::Entry*>(entry.second.get());
        if (sysEntry->sourceDumpId() == INVALID_SOURCE_ID)
        {
            sysEntry->update(ms, size, dumpId);
            return;
        }
    }

    // Get the id
    auto id = ++lastEntryId;
    auto idString = std::to_string(id);
    auto objPath = fs::path(baseEntryPath) / idString;
    entries.insert(std::make_pair(
        id, std::make_unique<system::Entry>(bus, objPath.c_str(), id, ms, size,
                                            dumpId, phosphor::dump::OperationStatus::Completed,  *this)));
}

sdbusplus::message::object_path
    Manager::createDump(std::map<std::string, std::string> params)
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
    constexpr auto DIAG_MOD_TARGET = "obmc-host-diagnostic-mode@0.target";

    if (!params.empty())
    {
        log<level::INFO>("System dump accepts no additional parameters");
    }

    auto b = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append(DIAG_MOD_TARGET); // unit to activate
    method.append("replace");
    bus.call_noreply(method);

    auto id = ++lastEntryId;
    auto idString = std::to_string(id);
    auto objPath = fs::path(baseEntryPath) / idString;
    entries.insert(std::make_pair(
        id, std::make_unique<system::Entry>(bus, objPath.c_str(), id, 0, 0,
                                            INVALID_SOURCE_ID, phosphor::dump::OperationStatus::InProgress,   *this)));
    return std::string(objPath.c_str());
}

} // namespace system
} // namespace dump
} // namespace phosphor
