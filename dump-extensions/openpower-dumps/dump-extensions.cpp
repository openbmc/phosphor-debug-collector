#include "config.h"

#include "dump-extensions.hpp"

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_manager_openpower.hpp"
#include "dump_manager_resource.hpp"

namespace phosphor
{
namespace dump
{

void loadExtensions(sdbusplus::bus_t& bus, DumpManagerList& dumpList)
{
    dumpList.push_back(std::make_unique<openpower::dump::Manager>(
        bus, openpower::dump::OP_DUMP_OBJ_PATH,
        openpower::dump::OP_BASE_ENTRY_PATH));
    dumpList.push_back(std::make_unique<openpower::dump::resource::Manager>(
        bus, RESOURCE_DUMP_OBJPATH, RESOURCE_DUMP_OBJ_ENTRY));
}
} // namespace dump
} // namespace phosphor
