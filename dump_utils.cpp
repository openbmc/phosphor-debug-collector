#include "dump_utils.hpp"

#include "dump_types.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace dump
{

std::string getService(sdbusplus::bus_t& bus, const std::string& path,
                       const std::string& interface)
{
    constexpr auto objectMapperName = "xyz.openbmc_project.ObjectMapper";
    constexpr auto objectMapperPath = "/xyz/openbmc_project/object_mapper";

    auto method = bus.new_method_call(objectMapperName, objectMapperPath,
                                      objectMapperName, "GetObject");

    method.append(path);
    method.append(std::vector<std::string>({interface}));

    std::vector<std::pair<std::string, std::vector<std::string>>> response;

    try
    {
        auto reply = bus.call(method);
        reply.read(response);
        if (response.empty())
        {
            lg2::error(
                "Error in mapper response for getting service name, PATH: "
                "{PATH}, INTERFACE: {INTERFACE}",
                "PATH", path, "INTERFACE", interface);
            return std::string{};
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Error in mapper method call, errormsg: {ERROR}, "
                   "PATH: {PATH}, INTERFACE: {INTERFACE}",
                   "ERROR", e, "PATH", path, "INTERFACE", interface);
        throw;
    }
    return response[0].first;
}

std::string validateDumpType(const std::string& type,
                             const std::string& category)
{
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    using Argument = xyz::openbmc_project::Common::InvalidArgument;
    // Dump type user will be return if type is empty
    std::string dumpType = "user";
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
