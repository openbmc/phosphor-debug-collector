#include "config.h"

#include "dump-extensions.hpp"

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

<<<<<<< HEAD
#include "dump_manager_hardware.hpp"
=======
>>>>>>> 784f820... OpenPOWER: Add support for Hostboot dump.
#include "dump_manager_hostboot.hpp"
#include "dump_manager_resource.hpp"
#include "dump_manager_system.hpp"
#include "dump_utils.hpp"

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

    dumpList.push_back(std::make_unique<openpower::dump::hostboot::Manager>(
        bus, event, HOSTBOOT_DUMP_OBJPATH, HOSTBOOT_DUMP_OBJ_ENTRY,
        HOSTBOOT_DUMP_PATH));

    try
    {
        std::filesystem::create_directories(HARDWARE_DUMP_PATH);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Failed to create hardware dump directory({})",
                        HARDWARE_DUMP_PATH)
                .c_str());
        throw std::runtime_error("Failed to create hardware dump directory");
    }

    dumpList.push_back(std::make_unique<openpower::dump::hardware::Manager>(
        bus, event, HARDWARE_DUMP_OBJPATH, HARDWARE_DUMP_OBJ_ENTRY,
        HARDWARE_DUMP_PATH));
}
} // namespace dump
} // namespace phosphor
