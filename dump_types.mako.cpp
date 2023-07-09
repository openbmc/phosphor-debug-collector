## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#include "dump_types.hpp"
namespace phosphor
{
namespace dump
{
DUMP_TYPE_TABLE dumpTypeTable = {
% for item in DUMP_TYPE_TABLE:
  % for key, values in item.items():
    {"${key}", {DumpTypes::${values[0].upper()}, "${values[1]}"}},
  % endfor
% endfor
};

DUMP_TYPE_TO_STRING_MAP dumpTypeToStringMap = {
% for item in DUMP_TYPE_TABLE:
  % for key, values in item.items():
    {DumpTypes::${values[0].upper()}, "${values[0]}"},
  % endfor
% endfor
};

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

