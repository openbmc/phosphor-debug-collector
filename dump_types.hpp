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
    CORE,
    RAMOOPS,
    ELOG,
};

// A table of dump types
using DUMP_TYPE_TABLE =
    std::unordered_map<DUMP_TYPE, std::pair<DumpTypes, DUMP_CATEGORY>>;

// Mapping between dump type and dump collection type string
using DUMP_TYPE_TO_STRING_MAP =
    std::unordered_map<DumpTypes, DUMP_COLLECTION_TYPE>;

extern DUMP_TYPE_TABLE dumpTypeTable;
extern DUMP_TYPE_TO_STRING_MAP dumpTypeToStringMap;

/**
 * @brief Converts a DumpTypes enum value to dump name.
 *
 * @param[in] dt The DumpTypes value to be converted.
 * @return Name of the dump as strung, std::nullopt if not found.
 */
std::optional<std::string> dumpTypeToString(const DumpTypes& dt);

/**
 * @brief Converts dump name to its corresponding DumpTypes enum value.
 *
 * @param[in] str The string to be converted to a DumpTypes value.
 * @return The DumpTypes value that corresponds to the name or std::nullopt if
 * not found.
 */
std::optional<DumpTypes> stringToDumpType(const std::string& str);

} // namespace dump
} // namespace phosphor
