## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#pragma once

#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

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

using ErrorType = std::string;
using Error = std::string;
using ErrorList = std::vector<Error>;
using ErrorMap = std::unordered_map<ErrorType, ErrorList>;

// Dump types
<% enum_values = set() %>
enum class DumpTypes {
% for item in DUMP_TYPE_TABLE:
  % for key, values in item.items():
    % if values[0].upper() not in enum_values:
        ${values[0].upper()},
        <% enum_values.add(values[0].upper()) %>
    % endif
  % endfor
% endfor
% for key in ERROR_TYPE_DICT:
    % if key.upper() not in enum_values:
        ${key.upper()},
        <% enum_values.add(key.upper()) %>
    % endif
% endfor
};

// A table of dump types
using DUMP_TYPE_TABLE =
    std::unordered_map<DUMP_TYPE, std::pair<DumpTypes, DUMP_CATEGORY>>;

// Mapping between dump type and dump collection type string
using DUMP_TYPE_TO_STRING_MAP =
    std::unordered_map<DumpTypes, DUMP_COLLECTION_TYPE>;

/**
 * @brief Converts a DumpTypes enum value to dump name.
 *
 * @param[in] dumpType The DumpTypes value to be converted.
 * @return Name of the dump as string, std::nullopt if not found.
 */
std::optional<std::string> dumpTypeToString(const DumpTypes& dumpType);

/**
 * @brief Converts dump name to its corresponding DumpTypes enum value.
 *
 * @param[in] str The string to be converted to a DumpTypes value.
 * @return The DumpTypes value that corresponds to the name or std::nullopt if
 * not found.
 */
std::optional<DumpTypes> stringToDumpType(const std::string& str);

/**
 * @brief Validates dump type and returns corresponding collection type
 *
 * This function checks the provided dump type against the specified category.
 * If the dump type is empty, it defaults to "user". If the dump type does not
 * exist or does not match with the specified category, it logs an error and
 * throws an InvalidArgument exception.
 *
 * @param[in] type - The dump type to be validated.
 * @param[in] category - The category to match against the dump type.
 *
 * @return The corresponding dump collection type if the dump type and category
 * match an entry in the dumpTypeTable. If the type is empty or does not match
 * any entry, it returns "user".
 *
 * @throws InvalidArgument - Thrown if the type does not match the specified
 * category or does not exist in the table.
 */
DumpTypes validateDumpType(const std::string& type,
                           const std::string& category);

/**
 * @brief Checks if the provided error type is valid.
 *
 * This function verifies the validity of the error type by checking if it
 * exists in the error map.
 *
 * @param[in] errorType - The string representation of the error type to
 * validate.
 *
 * @return True if the error type exists in the error map, False otherwise.
 */
bool isErrorTypeValid(const std::string& errorType);

/**
 * @brief Finds the error type based on the provided error string.
 *
 * This function searches the error map for the provided error string.
 * If it finds the string in the list of errors for a specific error type,
 * it returns that error type.
 *
 * @param[in] errString - The string representation of the error to search for.
 *
 * @return An optional containing the error type if found. If the error string
 * is not found in the map, returns an empty optional.
 */
std::optional<ErrorType> findErrorType(const std::string& errString);

} // namespace dump
} // namespace phosphor
