#include "config.h"

#include "dump_manager_system.hpp"

#include "system_dump_entry.hpp"

namespace phosphor
{
namespace dump
{
namespace system
{

void Manager::notify(NewDump::DumpType, uint32_t dumpId, uint64_t size)
{
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

} // namespace system
} // namespace dump
} // namespace phosphor
