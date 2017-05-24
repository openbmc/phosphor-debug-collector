#include <stdexcept>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <sys/inotify.h>
#include <unistd.h>
#include "config.h"
#include "dump_watch.hpp"

namespace phosphor
{
namespace dump
{

using namespace phosphor::dump;
using namespace std::string_literals;

Watch::Watch(sd_event* loop)
{
    fd = inotify_init1(IN_NONBLOCK);
    if (-1 == fd)
    {
        // Store a copy of errno, because the string creation below will
        // invalidate errno due to one more system calls.
        // TODO Add Phosphor log
        //auto error = errno;
    }

    wd = inotify_add_watch(fd, CORE_FILE_DIR, IN_CLOSE_WRITE);
    if (-1 == wd)
    {
        //auto error = errno;
        close(fd);
        // TODO Add Phosphor log
    }

    auto rc = sd_event_add_io(loop,
                              nullptr,
                              fd,
                              EPOLLIN,
                              callback,
                              this);
    if (0 > rc)
    {
        // Failed to add to event loop
        // TODO Add Phosphor log
    }
}

Watch::~Watch()
{
    if ((-1 != fd) && (-1 != wd))
    {
        inotify_rm_watch(fd, wd);
        close(fd);
    }
}

int Watch::callback(sd_event_source* s,
                    int fd,
                    uint32_t revents,
                    void* userdata)
{
    if (!(revents & EPOLLIN))
    {
        return 0;
    }

    constexpr auto maxBytes = 1024;
    uint8_t buffer[maxBytes];
    auto bytes = read(fd, buffer, maxBytes);
    if (0 > bytes)
    {
        //auto error = errno;
        //failed to read inotify event
        //TODO Add Phosphor log
    }

    auto offset = 0;
    std::vector<std::string> corelist;

    while (offset < bytes)
    {
        auto event = reinterpret_cast<inotify_event*>(&buffer[offset]);
        if ((event->mask & IN_CLOSE_WRITE) && !(event->mask & IN_ISDIR))
        {
            corelist.push_back(event->name);
        }

        offset += offsetof(inotify_event, name) + event->len;
    }

    //TODO Enable Dump collection function here

    return 0;
}

} // namespace dump
} // namespace phosphor
