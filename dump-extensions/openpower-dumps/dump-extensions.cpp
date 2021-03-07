#include "config.h"

#include "dump-extensions.hpp"

#include "dump_manager_hostboot.hpp"
#include "dump_manager_resource.hpp"
#include "dump_manager_system.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{

void loadExtensions(sdbusplus::bus::bus& bus, DumpManagerList& dumpList)
{
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

    dumpList.push_back(std::make_unique<openpower::dump::system::Manager>(
        bus, SYSTEM_DUMP_OBJPATH, SYSTEM_DUMP_OBJ_ENTRY));
    dumpList.push_back(std::make_unique<openpower::dump::resource::Manager>(
        bus, RESOURCE_DUMP_OBJPATH, RESOURCE_DUMP_OBJ_ENTRY));

    sd_event* event = nullptr;
    auto rc = sd_event_default(&event);
    if (rc < 0)
    {
        log<level::ERR>("Error occurred during the sd_event_default",
                        entry("RC=%d", rc));
        elog<InternalFailure>();
        return;
    }
    phosphor::dump::EventPtr eventP{event};
    event = nullptr;
    dumpList.push_back(std::make_unique<openpower::dump::hostboot::Manager>(
        bus, eventP, HOSTBOOT_DUMP_OBJPATH, HOSTBOOT_DUMP_OBJ_ENTRY,
        HOSTBOOT_DUMP_PATH));
}
} // namespace dump
} // namespace phosphor
