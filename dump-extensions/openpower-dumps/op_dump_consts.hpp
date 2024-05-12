#pragma once

namespace openpower::dump
{

constexpr uint32_t INVALID_SOURCE_ID = 0xFFFFFFFF;
constexpr auto OP_BASE_ENTRY_PATH = "/xyz/openbmc_project/dump/system/entry";
constexpr auto OP_DUMP_OBJ_PATH = "/xyz/openbmc_project/dump/system";
constexpr auto OP_DUMP_PATH = "/var/lib/phosphor-debug-collector/opdump";

} // namespace openpower::dump
