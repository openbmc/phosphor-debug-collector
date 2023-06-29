#include "dump_types.hpp"
namespace phosphor
{
namespace dump
{
DUMP_TYPE_TABLE dumpTypeTable = {
    {"xyz.openbmc_project.Dump.Create.DumpType.UserRequested",
     {DumpTypes::USER, "BMC_DUMP"}},
    {"xyz.openbmc_project.Dump.Create.DumpType.ApplicationCored",
     {DumpTypes::CORE, "BMC_DUMP"}}};

DUMP_TYPE_TO_STRING_MAP dumpTypeToStringMap = {{DumpTypes::USER, "user"},
                                               {DumpTypes::CORE, "core"}};
} // namespace dump
} // namespace phosphor
