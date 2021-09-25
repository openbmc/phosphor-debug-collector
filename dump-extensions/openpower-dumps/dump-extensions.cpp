#include "config.h"

#include "dump-extensions.hpp"

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_manager_resource.hpp"
#include "dump_manager_system.hpp"

namespace phosphor
{
namespace dump
{
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

void loadExtensions(sdbusplus::bus_t& bus, DumpManagerList& dumpList)
{
    try
    {
        std::filesystem::create_directories(SYSTEM_DUMP_SERIAL_PATH);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            fmt::format(
                "Failed to create system dump serial path({}), errormsg({})",
                SYSTEM_DUMP_SERIAL_PATH, e.what())
                .c_str());
        elog<InternalFailure>();
    }

    dumpList.push_back(std::make_unique<openpower::dump::system::Manager>(
        bus, SYSTEM_DUMP_OBJPATH, SYSTEM_DUMP_OBJ_ENTRY));

    try
    {
        std::filesystem::create_directories(RESOURCE_DUMP_SERIAL_PATH);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            fmt::format(
                "Failed to create resource dump serial path({}), errormsg({})",
                RESOURCE_DUMP_SERIAL_PATH, e.what())
                .c_str());
        elog<InternalFailure>();
    }

    dumpList.push_back(std::make_unique<openpower::dump::resource::Manager>(
        bus, RESOURCE_DUMP_OBJPATH, RESOURCE_DUMP_OBJ_ENTRY));
}
} // namespace dump
} // namespace phosphor
