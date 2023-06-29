#include "dump_types.hpp"
namespace phosphor
{
namespace dump
{
DUMP_TYPE_TABLE dumpTypeTable = {
    {"xyz.openbmc_project.Dump.Create.DumpType.UserRequested",
     {"user", "BMC_DUMP"}},
    {"xyz.openbmc_project.Dump.Create.DumpType.ApplicationCored",
     {"core", "BMC_DUMP"}},
    {"xyz.openbmc_project.Dump.Create.DumpType.Ramoops",
     {"ramopps", "BMC_DUMP"}}};
}
} // namespace phosphor
