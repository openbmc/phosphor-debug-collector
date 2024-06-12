#include "config.h"

#include "dump_utils.hpp"

#include "dump_types.hpp"

#include <phosphor-logging/lg2.hpp>

#include <ctime>
#include <filesystem>
#include <optional>
#include <regex>
#include <tuple>

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

std::optional<std::tuple<uint32_t, uint64_t, uint64_t>>
    extractDumpDetails(const std::filesystem::path& file)
{
    static constexpr auto ID_POS = 1;
    static constexpr auto EPOCHTIME_POS = 2;
    std::regex file_regex("obmcdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)");

    std::smatch match;
    std::string name = file.filename().string();

    if (!((std::regex_search(name, match, file_regex)) && (match.size() > 0)))
    {
        lg2::error("Invalid Dump file name, FILENAME: {FILENAME}", "FILENAME",
                   file);
        return std::nullopt;
    }

    auto idString = match[ID_POS];
    uint64_t timestamp = stoull(match[EPOCHTIME_POS]) * 1000 * 1000;

    return std::make_tuple(stoul(idString), timestamp,
                           std::filesystem::file_size(file));
}

} // namespace dump
} // namespace phosphor
