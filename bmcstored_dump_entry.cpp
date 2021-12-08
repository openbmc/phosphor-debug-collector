#include "bmc_dump_entry.hpp"
#include "dump_manager.hpp"
#include "dump_offload.hpp"

#include <fmt/core.h>

#include <phosphor-logging/log.hpp>

#include <future>

namespace phosphor
{
namespace dump
{
namespace bmc_stored
{
using namespace phosphor::logging;

static std::future<uint32_t> asyncThread;
uint32_t Entry::downloadHelper()
{
    phosphor::dump::offload::requestOffload(file, id, offloadUri());
    log<level::ERR>(fmt::format("offload complete id({})", id).c_str());
    offloaded(true);
    return 0;
}

void Entry::delete_()
{
    // Delete Dump file from Permanent location
    try
    {
        std::filesystem::remove_all(
            std::filesystem::path(path()).parent_path());
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Log Error message and continue
        log<level::ERR>(
            fmt::format("Failed to delete dump file({}), errormsg({})", path(),
                        e.what())
                .c_str());
    }

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
}

void Entry::initiateOffload(std::string uri)
{
    log<level::ERR>(
        fmt::format("offload started id({}) uri({})", id, uri).c_str());
    if (asyncThread.valid())
    {
        asyncThread.get();
    }
    phosphor::dump::Entry::initiateOffload(uri);
    asyncThread = std::async(std::launch::async,
                             &phosphor::dump::bmc::Entry::downloadHelper, this);
}

} // namespace bmc_stored
} // namespace dump
} // namespace phosphor
