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

void Entry::initiateOffload()
{
    pldm::PLDMInterface pldmIntf;
    if (pldmIntf.sendGetSysDumpCmd(sourceDumpId()) == pldm::CmdStatus::failure)
    {
        log<level::ERR>("Failed to send offload request",
                        entry("DUMPID=%d",id),
                        entry("SOURCEDUMPID=%d",sourceDumpId()));
        elog<InternalFailure>();
    }
}

} // namespace system
} // namespace dump
} // namespace phosphor
