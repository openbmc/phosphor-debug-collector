#include "config.h"

#include "dump-extensions.hpp"

#include "dump_manager_hostboot.hpp"
#include "dump_manager_resource.hpp"
#include "dump_manager_system.hpp"

namespace phosphor
{
namespace dump
{

void loadExtensions(sdbusplus::bus::bus& bus, const EventPtr& event,
                    phosphor::dump::internal::Manager& dumpInternalMgr,
                    DumpManagerList& dumpList)
{

    dumpList.push_back(std::make_unique<openpower::dump::system::Manager>(
        bus, SYSTEM_DUMP_OBJPATH, SYSTEM_DUMP_OBJ_ENTRY));
    dumpList.push_back(std::make_unique<openpower::dump::resource::Manager>(
        bus, RESOURCE_DUMP_OBJPATH, RESOURCE_DUMP_OBJ_ENTRY));
    dumpList.push_back(std::make_unique<openpower::dump::hostboot::Manager>(
        bus, event, HOSTBOOT_DUMP_OBJPATH, HOSTBOOT_DUMP_OBJ_ENTRY,
        HOSTBOOT_DUMP_PATH, dumpInternalMgr, ""));
}

void loadFileWatchList(const EventPtr&, FileWatchManagerList&)
{
}
} // namespace dump
} // namespace phosphor
