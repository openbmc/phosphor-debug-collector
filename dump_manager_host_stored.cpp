#include "config.h"

#include "dump_manager_host_stored.hpp"

#include "bmc_dump_entry.hpp"
#include "dump_internal.hpp"
#include "system_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <sys/inotify.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <regex>

namespace phosphor
{
namespace dump
{
namespace host_stored
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

void Manager::notify(NewDump::DumpType, uint32_t dumpId, uint64_t size)
{
    // Get the timestamp
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();
    // Get the id
    auto id = lastEntryId + 1;
    auto idString = std::to_string(id);
    auto objPath = fs::path(OBJ_ENTRY) / idString;
    entries.insert(std::make_pair(
        id, std::make_unique<system::Entry>(bus, objPath.c_str(), id, ms, size,
                                            dumpId, *this)));
    lastEntryId++;
}

} // namespace host_stored
} // namespace dump
} // namespace phosphor
