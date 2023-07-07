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

std::optional<std::tuple<uint32_t, uint64_t, uint64_t>> extractDumpDetails(
    const std::filesystem::path& file)
{
    std::regex file_regex(BMC_DUMP_FILENAME_REGEX);

    std::smatch match;
    std::string name = file.filename().string();

    if (!((std::regex_search(name, match, file_regex)) && (match.size() > 0)))
    {
        lg2::error("Invalid Dump file name, FILENAME: {FILENAME}", "FILENAME",
                   file);
        return std::nullopt;
    }

    auto idString = match[FILENAME_DUMP_ID_POS];
    auto ts = match[FILENAME_EPOCHTIME_POS];
    uint64_t timestamp = 0;

    if (TIMESTAMP_FORMAT == 1)
    {
        timestamp = timeToEpoch(ts);
    }
    else
    {
        timestamp = stoull(ts) * 1000 * 1000;
    }

    return std::make_tuple(stoul(idString), timestamp,
                           std::filesystem::file_size(file));
}

} // namespace dump
} // namespace phosphor
