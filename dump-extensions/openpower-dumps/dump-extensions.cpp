#include "config.h"

#include "dump-extensions.hpp"

#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "dump_manager_hostdump.hpp"
#include "dump_manager_resource.hpp"
#include "dump_manager_system.hpp"
#include "dump_utils.hpp"
#include "hardware_dump_entry.hpp"
#include "hostboot_dump_entry.hpp"

#include <fmt/core.h>

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
constexpr auto HB_DUMP_FILENAME_REGEX =
    "hbdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)";

constexpr auto HARDWARE_DUMP_FILENAME_REGEX =
    "hwdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)";

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
        HOSTBOOT_DUMP_PATH, HB_DUMP_FILENAME_REGEX, "hbdump",
        HOSTBOOT_DUMP_TMP_FILE_DIR, HOSTBOOT_DUMP_MAX_SIZE,
        HOSTBOOT_DUMP_MIN_SPACE_REQD, HOSTBOOT_DUMP_TOTAL_SIZE));

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

    dumpList.push_back(std::make_unique<openpower::dump::hostdump::Manager<
                           openpower::dump::hardware::Entry>>(
        bus, event, HARDWARE_DUMP_OBJPATH, HARDWARE_DUMP_OBJ_ENTRY,
        HARDWARE_DUMP_PATH, HARDWARE_DUMP_FILENAME_REGEX, "hwdump",
        HOSTBOOT_DUMP_TMP_FILE_DIR, HARDWARE_DUMP_MAX_SIZE,
        HARDWARE_DUMP_MIN_SPACE_REQD, HARDWARE_DUMP_TOTAL_SIZE));
}
} // namespace dump
} // namespace phosphor
