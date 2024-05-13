#pragma once

namespace openpower::dump
{

constexpr uint32_t INVALID_SOURCE_ID = 0xFFFFFFFF;
constexpr auto OP_BASE_ENTRY_PATH = "/xyz/openbmc_project/dump/system/entry";
constexpr auto OP_DUMP_OBJ_PATH = "/xyz/openbmc_project/dump/system";
constexpr auto OP_DUMP_PATH = "/var/lib/phosphor-debug-collector/opdump";

constexpr uint32_t DUMP_ID_PREFIX_MASK = 0xF0000000;
constexpr uint32_t DUMP_ID_DUMP_ID_MASK = 0x0FFFFFFF;
constexpr uint32_t SYSTEM_DUMP_ID_PREFIX = 0xA0000000;
constexpr uint32_t RESOURCE_DUMP_ID_PREFIX = 0xB0000000;
constexpr uint32_t HARDWARE_DUMP_ID_PREFIX = 0x00000000;
constexpr uint32_t HOSTBOOT_DUMP_ID_PREFIX = 0x20000000;
constexpr uint32_t SBE_DUMP_ID_PREFIX = 0x30000000;

} // namespace openpower::dump
