#include "config.h"

#include "dump_manager_bmc.hpp"

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
namespace BMC
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

uint32_t Manager::createDump()
{
    std::vector<std::string> paths;
    return captureDump(phosphor::dump::BMC_stored::Type::UserRequested, paths);
}

} // namecpace BMC
} // namespace dump
} // namespace phosphor
