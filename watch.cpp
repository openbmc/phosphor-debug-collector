#include "watch.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>

#include <span>

namespace phosphor
{
namespace dump
{
namespace inotify
{

using namespace std::string_literals;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

Watch::~Watch()
{
    if ((fd() >= 0) && (wd >= 0))
    {
        sd_event_source_unref(source);
        inotify_rm_watch(fd(), wd);
    }
}

Watch::Watch(const EventPtr& eventObj, const int flags, const uint32_t mask,
             const uint32_t events, const std::filesystem::path& path,
             UserType userFunc) :
    flags(flags), mask(mask), events(events), path(path), fd(inotifyInit()),
    userFunc(userFunc)
{
    // Check if watch DIR exists.
    if (!std::filesystem::is_directory(path))
    {
        lg2::error("Watch directory doesn't exist, DIR: {DIRECTORY}",
                   "DIRECTORY", path);
        elog<InternalFailure>();
    }

    wd = inotify_add_watch(fd(), path.c_str(), mask);
    if (-1 == wd)
    {
        auto error = errno;
        lg2::error(
            "Error occurred during the inotify_add_watch call, errno: {ERRNO}",
            "ERRNO", error);
        elog<InternalFailure>();
    }

    auto rc =
        sd_event_add_io(eventObj.get(), &source, fd(), events, callback, this);
    if (0 > rc)
    {
        // Failed to add to event loop
        lg2::error("Error occurred during the sd_event_add_io call, rc: {RC}",
                   "RC", rc);
        elog<InternalFailure>();
    }
}

int Watch::inotifyInit()
{
    auto fd = inotify_init1(flags);

    if (-1 == fd)
    {
        auto error = errno;
        lg2::error("Error occurred during the inotify_init1, errno: {ERRNO}",
                   "ERRNO", error);
        elog<InternalFailure>();
    }

    return fd;
}

int Watch::callback(sd_event_source*, int fd, uint32_t revents, void* userdata)
{
    auto userData = static_cast<Watch*>(userdata);

    if ((revents & userData->events) == 0U)
    {
        return 0;
    }

    // Maximum inotify events supported in the buffer
    constexpr auto maxBytes = sizeof(struct inotify_event) + NAME_MAX + 1;
    uint8_t buffer[maxBytes];

    std::span<char> bufferSpan(reinterpret_cast<char*>(buffer), maxBytes);
    auto bytes = read(fd, bufferSpan.data(), bufferSpan.size());
    if (0 > bytes)
    {
        // Failed to read inotify event
        // Report error and return
        auto error = errno;
        lg2::error("Error occurred during the read, errno: {ERRNO}", "ERRNO",
                   error);
        report<InternalFailure>();
        return 0;
    }

    auto offset = 0;

    UserMap userMap;

    while (offset < bytes)
    {
        auto event = reinterpret_cast<inotify_event*>(&buffer[offset]);
        auto mask = event->mask & userData->mask;

        if (mask != 0U)
        {
            userMap.emplace((userData->path / event->name), mask);
        }

        offset += offsetof(inotify_event, name) + event->len;
    }

    // Call user call back function in case valid data in the map
    if (!userMap.empty())
    {
        userData->userFunc(userMap);
    }

    return 0;
}

} // namespace inotify
} // namespace dump
} // namespace phosphor
