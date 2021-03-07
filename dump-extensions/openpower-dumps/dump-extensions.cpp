#include "config.h"

#include "dump-extensions.hpp"

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_manager_hostdump.hpp"
#include "dump_manager_resource.hpp"
#include "dump_manager_system.hpp"
#include "dump_utils.hpp"
#include "hostboot_dump_entry.hpp"

#include <fmt/core.h>

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{

using namespace phosphor::logging;
void loadExtensions(sdbusplus::bus::bus& bus,
                    const phosphor::dump::EventPtr& event,
                    DumpManagerList& dumpList)
{
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
                .c_str());
        throw std::runtime_error("Failed to create hostboot dump directory");
    }
    dumpList.push_back(std::make_unique<openpower::dump::hostdump::Manager<
                           openpower::dump::hostboot::Entry>>(
        bus, event, HOSTBOOT_DUMP_OBJPATH, HOSTBOOT_DUMP_OBJ_ENTRY,
        HOSTBOOT_DUMP_PATH, "hbdump", HOSTBOOT_DUMP_TMP_FILE_DIR,
        HOSTBOOT_DUMP_MAX_SIZE, HOSTBOOT_DUMP_MIN_SPACE_REQD,
        HOSTBOOT_DUMP_TOTAL_SIZE));
}
} // namespace dump
} // namespace phosphor
