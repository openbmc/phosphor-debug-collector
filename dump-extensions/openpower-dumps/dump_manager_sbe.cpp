#include "config.h"

#include "dump_manager_sbe.hpp"

#include "op_dump_util.hpp"
#include "sbe_dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <fmt/core.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

#include <ctime>
#include <regex>

namespace openpower
{
namespace dump
{
namespace sbe
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

constexpr auto INVALID_DUMP_SIZE = 0;

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    if (!params.empty())
    {
        log<level::ERR>(fmt::format("SBE dump accepts no additional "
                                    "parameters, number of parameters({})",
                                    params.size())
                            .c_str());
        throw std::runtime_error("SBE dump accepts no additional parameters");
    }

    // Check dump policy
    util::isOPDumpsEnabled();

    uint32_t id = ++lastEntryId;
    // Entry Object path.
    auto objPath = std::filesystem::path(baseEntryPath) / std::to_string(id);

    std::time_t timeStamp = std::time(nullptr);
    createEntry(id, objPath, timeStamp, 0, std::string(),
                phosphor::dump::OperationStatus::InProgress);

    return objPath.string();
}

void Manager::createEntry(const uint32_t id, const std::string objPath,
                          const uint64_t ms, uint64_t fileSize,
                          const std::filesystem::path& file,
                          phosphor::dump::OperationStatus status)
{
    try
    {
        entries.insert(
            std::make_pair(id, std::make_unique<openpower::dump::sbe::Entry>(
                                   bus, objPath.c_str(), id, ms, fileSize, file,
                                   status, *this)));
    }
    catch (const std::invalid_argument& e)
    {
        log<level::ERR>(fmt::format("Error in creating SBE dump entry, "
                                    "errormsg({}), OBJECTPATH({}), ID({}), "
                                    "timestamp({}), filesize({}), status({})",
                                    e.what(), objPath.c_str(), id, ms, fileSize,
                                    status)
                            .c_str());
        throw std::runtime_error("Error in creating SBE dump entry");
    }
}

void Manager::notify(uint32_t dumpId, uint64_t)
{
    // Get Dump size.
    // TODO #ibm-openbmc/issues/3061
    // Dump request will be rejected if there is not enough space told
    // one complete dump, change this behavior to crate a partial dump
    // with available space.
    auto size = getAllowedSize();
    std::vector<std::string> paths;
    try
    {
        util::captureDump(dumpId, size, SBE_DUMP_TMP_FILE_DIR, dumpDir,
                          "sbedump", eventLoop);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Failed to package SBE dump: dump({id}) errorMsg({})",
                        dumpId, e.what())
                .c_str());
        elog<InternalFailure>();
    }
}

} // namespace sbe
} // namespace dump
} // namespace openpower
