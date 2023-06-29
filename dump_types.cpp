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
    {"xyz.openbmc_project.Dump.Create.DumpType.UserRequested",
     {DumpTypes::USER, "BMC_DUMP"}},
    {"xyz.openbmc_project.Dump.Create.DumpType.ApplicationCored",
     {DumpTypes::CORE, "BMC_DUMP"}},
    {"xyz.openbmc_project.Dump.Create.DumpType.Ramoops",
     {DumpTypes::RAMOOPS, "BMC_DUMP"}},
    {"xyz.openbmc_project.Dump.Create.DumpType.ErrorLog",
     {DumpTypes::ELOG, "elog"}}};

DUMP_TYPE_TO_STRING_MAP dumpTypeToStringMap = {
    {DumpTypes::USER, "user"},
    {DumpTypes::CORE, "core"},
    {DumpTypes::RAMOOPS, "ramoops"},
    {DumpTypes::ELOG, "elog"},
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

} // namespace dump
} // namespace phosphor
