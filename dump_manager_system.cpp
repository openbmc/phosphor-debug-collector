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

void Manager::notify(NewDump::DumpType dumpType, uint32_t dumpId, uint64_t size)
{

    if (dumpType != NewDump::DumpType::System)
    {
        log<level::ERR>("Only system dump is supported",
                       entry("DUMPTYPE=%d", dumpType));
        return;
    }
    // Get the timestamp
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();
    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = fs::path(baseEntryPath) / idString;
    entries.insert(std::make_pair(
        id, std::make_unique<system::Entry>(bus, objPath.c_str(), id, ms, size,
                                            dumpId, *this)));
    lastEntryId++;
}

uint32_t Manager::createDump()
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
    constexpr auto DIAG_MOD_TARGET = "obmc-host-diagnostic-mode@0.target";
    auto b = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append(DIAG_MOD_TARGET); // unit to activate
    method.append("replace");
    bus.call_noreply(method);
    return ++lastEntryId;
}

} // namespace system
} // namespace dump
} // namespace phosphor
