#include "system_dump_entry.hpp"

#include "dump_manager.hpp"
#include "pldm_interface.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace system
{
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::dump::pldm;

void Entry::initiateOffload()
{   
    sendGetSysDumpCmd(sourceDumpId());
}

} // namespace system
} // namespace dump
} // namespace phosphor
