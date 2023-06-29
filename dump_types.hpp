#pragma once

#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>

namespace phosphor
{
namespace dump
{
// Overall dump category for example BMC dump
using DUMP_CATEGORY = std::string;

// Dump type
using DUMP_TYPE = std::string;

// Dump collection indicator
using DUMP_COLLECTION_TYPE = std::string;

// Dump types
enum class DumpTypes
{
    USER,
};

// A table of dump types
using DUMP_TYPE_TABLE =
    std::unordered_map<DUMP_TYPE, std::pair<DumpTypes, DUMP_CATEGORY>>;

// Mapping between dump type and dump collection type string
using DUMP_TYPE_TO_STRING_MAP =
    std::unordered_map<DumpTypes, DUMP_COLLECTION_TYPE>;

extern DUMP_TYPE_TABLE dumpTypeTable;
extern DUMP_TYPE_TO_STRING_MAP dumpTypeToStringMap;

inline std::optional<std::string> dumpTypeToString(const DumpTypes& dt)
{
    auto it = dumpTypeToStringMap.find(dt);
    if (it != dumpTypeToStringMap.end())
    {
        return it->second;
    }
    return std::nullopt;
}

inline std::optional<DumpTypes> stringToDumpType(const std::string& str)
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
