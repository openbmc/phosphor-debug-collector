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

std::optional<std::string> dumpTypeToString(const DumpTypes& dt)
{
    auto it = dumpTypeToStringMap.find(dt);
    if (it != dumpTypeToStringMap.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::optional<DumpTypes> stringToDumpType(const std::string& str)
{
    auto it = std::ranges::find_if(dumpTypeToStringMap,
                                   [&str](const auto& pair) {
        return pair.second == str;
    });

    if (it != dumpTypeToStringMap.end())
    {
        return it->first;
    }
    return std::nullopt;
}

} // namespace dump
} // namespace phosphor
