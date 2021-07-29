#include "config.h"

#include "dump-extensions.hpp"

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_manager_hostboot.hpp"
#include "dump_manager_resource.hpp"
#include "dump_manager_system.hpp"
#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{

void loadExtensions(sdbusplus::bus::bus& bus,
                    const phosphor::dump::EventPtr& event,
                    DumpManagerList& dumpList)
{
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

    dumpList.push_back(std::make_unique<openpower::dump::system::Manager>(
        bus, SYSTEM_DUMP_OBJPATH, SYSTEM_DUMP_OBJ_ENTRY));
    dumpList.push_back(std::make_unique<openpower::dump::resource::Manager>(
        bus, RESOURCE_DUMP_OBJPATH, RESOURCE_DUMP_OBJ_ENTRY));

    try
    {
        std::filesystem::create_directories(HOSTBOOT_DUMP_PATH);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Failed to create hostboot dump directory({})",
                        HOSTBOOT_DUMP_PATH)
                .c_str()) throw std::
            runtime_error("Failed to create hostboot dump directory");
    }

    dumpList.push_back(std::make_unique<openpower::dump::hostboot::Manager>(
        bus, event, HOSTBOOT_DUMP_OBJPATH, HOSTBOOT_DUMP_OBJ_ENTRY,
        HOSTBOOT_DUMP_PATH));
}
} // namespace dump
} // namespace phosphor
