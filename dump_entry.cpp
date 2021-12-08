#include "dump_entry.hpp"

#include "dump_manager.hpp"

#include <fcntl.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <cstring>

namespace phosphor
{
namespace dump
{

using namespace phosphor::logging;

void Entry::delete_()
{
    // Remove Dump entry D-bus object
    parent.erase(id);
}

sdbusplus::message::unix_fd Entry::getFileHandle()
{
    using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
    using metadata = xyz::openbmc_project::Common::File::Open;
    if (file.empty())
    {
        lg2::error("Failed to get file handle: File path is empty.");
        elog<sdbusplus::xyz::openbmc_project::Common::Error::Unavailable>();
    }

    if (fdCloseEventSource)
    {
        // Return the existing file descriptor
        return fdCloseEventSource->first;
    }

    int fd = open(file.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd == -1)
    {
        auto err = errno;
        lg2::error("Failed to open dump file: id: {ID} error: {ERRNO}", "ID",
                   id, "ERRNO", std::strerror(errno));
        elog<Open>(metadata::ERRNO(err), metadata::PATH(file.c_str()));
    }

    // Create a new Defer event source for closing this fd
    sdeventplus::Event event = sdeventplus::Event::get_default();
    auto eventSource = std::make_unique<sdeventplus::source::Defer>(
        event, [this](auto& /*source*/) { closeFD(); });

    // Store the file descriptor and event source in the optional pair
    fdCloseEventSource = std::make_pair(fd, std::move(eventSource));

    return fd;
}

} // namespace dump
} // namespace phosphor
