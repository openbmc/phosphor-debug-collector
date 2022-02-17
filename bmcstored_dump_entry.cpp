#include "bmc_dump_entry.hpp"
#include "dump_manager_bmcstored.hpp"
#include "dump_offload.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc_stored
{
using namespace phosphor::logging;

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
    log<level::INFO>(
        fmt::format("offload started id({}) uri({})", id, uri).c_str());

    phosphor::dump::Entry::initiateOffload(uri);
    pid_t pid = fork();
    if (pid == 0)
    {
        execl("/usr/bin/phosphor-offload-handler", "phosphor-offload-handler",
              "--id", std::to_string(id), "--path", path(), "--uri", uri,
              nullptr);
        throw std::runtime_error(
            "Dump offload failure: Error occured while starting offload");
    }
    if (pid > 0)
    {
        auto rc = sd_event_add_child(
            dynamic_cast<phosphor::dump::bmc_stored::Manager&>(parent)
                .eventLoop.get(),
            nullptr, pid, WEXITED | WSTOPPED, callback, this);
        if (0 > rc)
        {
            // Failed to add to event loop
            log<level::ERR>(fmt::format("Dump capture: Error occurred during "
                                        "the sd_event_add_child call, rc({})",
                                        rc)
                                .c_str());
            throw std::runtime_error("Dump capture: Error occurred during the "
                                     "sd_event_add_child call");
        }
    }
    else
    {
        auto error = errno;
        log<level::ERR>(
            fmt::format("Dump capture: Error occurred during fork, errno({})",
                        error)
                .c_str());
        throw std::runtime_error("Dump capture: Error occurred during fork");
    }
}

} // namespace bmc_stored
} // namespace dump
} // namespace phosphor
