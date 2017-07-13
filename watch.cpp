#include <phosphor-logging/elog-errors.hpp>

#include "xyz/openbmc_project/Common/error.hpp"
#include "watch.hpp"

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
        inotify_rm_watch(fd(), wd);
    }
}

Watch::Watch(const EventPtr& eventObj,
             const int flags,
             const uint32_t mask,
             const uint32_t events,
             const fs::path& path,
             UserType userFunc,
             const uint32_t childMask):
    flags(flags),
    mask(mask),
    events(events),
    path(path),
    fd(inotifyInit()),
    userFunc(userFunc),
    eventObj(eventObj.get()),
    childMask(childMask)
{
    // Check if watch DIR exists.
    if (!fs::is_directory(path))
    {
        log<level::ERR>("Watch directory doesn't exist",
                        entry("dir=%s", path.c_str()));
        elog<InternalFailure>();
    }

    //Adding Child mask event to support child directory level
    //file watch.
    wd = inotify_add_watch(fd(), path.c_str(), childMask | mask);
    if (-1 == wd)
    {
        auto error = errno;
        log<level::ERR>("Error occurred during the inotify_add_watch call",
                        entry("ERRNO=%d", error));
        elog<InternalFailure>();
    }

    auto rc = sd_event_add_io(eventObj.get(),
                              nullptr,
                              fd(),
                              events,
                              callback,
                              this);
    if (0 > rc)
    {
        // Failed to add to event loop
        log<level::ERR>("Error occurred during the sd_event_add_io call",
                        entry("rc=%d", rc));
        elog<InternalFailure>();
    }
}

int Watch::inotifyInit()
{
    auto fd = inotify_init1(flags);

    if (-1 == fd)
    {
        auto error = errno;
        log<level::ERR>("Error occurred during the inotify_init1",
                        entry("ERRNO=%d", error));
        elog<InternalFailure>();
    }

    return fd;
}

int Watch::callback(sd_event_source* s,
                    int fd,
                    uint32_t revents,
                    void* userdata)
{
    auto userData = static_cast<Watch*>(userdata);

    if (!(revents & userData->events))
    {
        return 0;
    }

    //Maximum inotify events supported in the buffer
    constexpr auto maxBytes = sizeof(struct inotify_event) + NAME_MAX + 1;
    uint8_t buffer[maxBytes];

    auto bytes = read(fd, buffer, maxBytes);
    if (0 > bytes)
    {
        //Failed to read inotify event
        //Report error and return
        auto error = errno;
        log<level::ERR>("Error occurred during the read",
                        entry("ERRNO=%d", error));
        report<InternalFailure>();
        return 0;
    }

    auto offset = 0;

    UserMap userMap;

    while (offset < bytes)
    {
        auto event = reinterpret_cast<inotify_event*>(&buffer[offset]);
        auto mask = event->mask & userData->mask;
        auto path = userData->path;

        // Checking the event related to new file creation.
        if (mask && !(event->mask & IN_ISDIR))
        {
            userMap.emplace(userData->path / event->name, mask);
        }

        // checking for event related to new directory creation
        // in the watch path.
        if ((event->mask & userData->childMask) &&
            (event->mask & IN_ISDIR))
        {
            //Create new inotify watch on child directory
            auto watchObj = std::make_unique<Watch>(
                                userData->eventObj,
                                userData->flags,
                                userData->mask,
                                userData->events,
                                (path / event->name).c_str(),
                                userData->userFunc);

            userData->watchMap.emplace((path / event->name),
                                       std::move(watchObj));
        }
        offset += offsetof(inotify_event, name) + event->len;
    }

    //Call user call back function incase valid data in the map
    if (!userMap.empty())
    {
        userData->userFunc(userMap);
    }
    return 0;
}

void Watch::erase(fs::path path)
{
    auto it = watchMap.find(path);
    if (it != watchMap.end())
    {
        //Delete Watch object.
        it->second.reset();
        watchMap.erase(path);
    }
}

} // namespace inotify
} // namespace dump
} // namespace phosphor
