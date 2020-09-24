#include "config.h"

#include "dump_manager_system.hpp"
#include "dump-extensions.hpp"

namespace phosphor
{
namespace dump
{

void loadExtensions(sdbusplus::bus::bus& bus, DumpManagerList& dumpList)
{
    
    dumpList.push_back(std::make_unique<phosphor::dump::system::Manager>(bus, SYSTEM_DUMP_OBJPATH, SYSTEM_DUMP_OBJ_ENTRY));
}
} // namespace dump
} // namespace phosphor
