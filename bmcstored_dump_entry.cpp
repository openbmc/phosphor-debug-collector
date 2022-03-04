#include "bmc_dump_entry.hpp"
#include "dump_manager_bmcstored.hpp"
#include "dump_offload.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc_stored
{
using namespace phosphor::logging;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

void Entry::delete_()
{
    if (isOffloadInProgress())
    {
        log<level::ERR>(
            fmt::format("Dump offload is in progress, cannot delete id({})", id)
                .c_str());
        elog<NotAllowed>(
            Reason("Dump offload is in progress, please try later"));
    }

    // Delete Dump file from Permanent location
    log<level::ERR>(
        fmt::format("Deleting dump id({}) path({})", id, path()).c_str());
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
    if ((isOffloadInProgress()) && (uri == offloadUri()))
    {
        log<level::ERR>(fmt::format("Another offload is in progress with same "
                                    "URI({}), cannot continue",
                                    offloadUri())
                            .c_str());
        elog<NotAllowed>(
            Reason("Another offload is in progress, please try later"));
    }

    setOffloadInProgress();

    log<level::INFO>(
        fmt::format("offload started id({}) uri({})", id, uri).c_str());

    phosphor::dump::Entry::initiateOffload(uri);
    pid_t pid = fork();
    if (pid == 0)
    {
        execl("/usr/bin/phosphor-offload-handler", "phosphor-offload-handler",
              "--id", std::to_string(id).c_str(), "--path", path().c_str(),
              "--uri", uri.c_str(), nullptr);
        log<level::ERR>(fmt::format("Dump offload failure: Error occured while "
                                    "starting offload id({})",
                                    id)
                            .c_str());
        std::exit(EXIT_FAILURE);
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
            log<level::ERR>(fmt::format("Dump offload: Error occurred during "
                                        "the sd_event_add_child call, rc({})",
                                        rc)
                                .c_str());
            throw std::runtime_error("Dump offload: Error occurred during the "
                                     "sd_event_add_child call");
        }
    }
    else
    {
        auto error = errno;
        log<level::ERR>(
            fmt::format("Dump offload: Error occurred during fork, errno({})",
                        error)
                .c_str());
        throw std::runtime_error("Dump offload: Error occurred during fork");
    }
}

} // namespace bmc_stored
} // namespace dump
} // namespace phosphor
