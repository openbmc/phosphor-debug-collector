#include "config.h"

#include "dump-extensions.hpp"

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_manager_resource.hpp"
#include "dump_manager_system.hpp"

namespace phosphor
{
namespace dump
{

void loadExtensions(sdbusplus::bus::bus& bus, DumpManagerList& dumpList)
{
    try
    {
        std::filesystem::create_directories(SYSTEM_DUMP_SERIAL_PATH);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            fmt::format(
                "Failed to create system dump directory({}), errormsg({})",
                SYSTEM_DUMP_SERIAL_PATH, e.what())
                .c_str());
        throw std::runtime_error("Failed to create system dump directory");
    }

    dumpList.push_back(std::make_unique<openpower::dump::system::Manager>(
        bus, SYSTEM_DUMP_OBJPATH, SYSTEM_DUMP_OBJ_ENTRY));
    dumpList.push_back(std::make_unique<openpower::dump::resource::Manager>(
        bus, RESOURCE_DUMP_OBJPATH, RESOURCE_DUMP_OBJ_ENTRY));
}
} // namespace dump
} // namespace phosphor
