## This file is a template.The comment below is emitted
## into the rendered file; feel free to edit this file.
//  !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#include "dump_types.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

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

<%
map_keys = set()
%>
DUMP_TYPE_TO_STRING_MAP dumpTypeToStringMap = {
% for item in DUMP_TYPE_TABLE:
  % for key, values in item.items():
    % if values[0].upper() not in map_keys:
        {DumpTypes::${values[0].upper()}, "${values[0]}"},
        <% map_keys.add(values[0].upper()) %>
    % endif
  % endfor
% endfor
% for key, values in ERROR_TYPE_DICT.items():
    % if key.upper() not in map_keys:
        {DumpTypes::${key.upper()}, "${key}"},
        <% map_keys.add(key.upper()) %>
    % endif
% endfor
};

const ErrorMap errorMap = {
% for key, errors in ERROR_TYPE_DICT.items():
    {"${key}", {
    % for error in errors:
        "${error}",
    % endfor
    }},
% endfor
};

std::optional<std::string> dumpTypeToString(const DumpTypes& dumpType)
{
    auto it = dumpTypeToStringMap.find(dumpType);
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

DumpTypes validateDumpType(const std::string& type, const std::string& category)
{
    using namespace phosphor::logging;
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;
    // Dump type user will be return if type is empty
    DumpTypes dumpType = DumpTypes::USER;
    if (type.empty())
    {
        return dumpType;
    }

    // Find any matching dump collection type for the category
    auto it = std::find_if(dumpTypeTable.begin(), dumpTypeTable.end(),
                           [&](const auto& pair) {
        return pair.first == type && pair.second.second == category;
    });

    if (it != dumpTypeTable.end())
    {
        dumpType = it->second.first;
    }
    else
    {
        lg2::error("An invalid dump type: {TYPE} passed", "TYPE", type);
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("BMC_DUMP_TYPE"),
                              Argument::ARGUMENT_VALUE(type.c_str()));
    }
    return dumpType;
}

bool isErrorTypeValid(const std::string& errorType)
{
    const auto it = errorMap.find(errorType);
    return it != errorMap.end();
}

std::optional<EType> findErrorType(const std::string& errString)
{
    for (const auto& [type, errorList] : errorMap)
    {
        auto error = std::find(errorList.begin(), errorList.end(), errString);
        if (error != errorList.end())
        {
            return type;
        }
    }
    return std::nullopt;
}

} // namespace dump
} // namespace phosphor
