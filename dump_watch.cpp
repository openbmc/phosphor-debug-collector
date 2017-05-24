#include <stdexcept>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <sys/inotify.h>
#include <unistd.h>
#include "config.h"
#include "dump_watch.hpp"
#include <experimental/filesystem>

namespace phosphor
{
namespace dump
{

using namespace std::string_literals;

CustomFD::~CustomFD()
{
    if (-1 != fd)
    {
        close(fd);
    }
}

Watch::~Watch()
{
    auto& inotFD = *(inotifyFD.get());
    if (-1 != inotFD())
    {
        inotify_rm_watch(inotFD(), wd);
        close(inotFD());
    }
}

Watch::Watch(sd_event* loop)
{
    auto fd = inotify_init1(IN_NONBLOCK);

    if (-1 == fd)
    {
        //TODO Add phosphor log.
        // Store a copy of errno, because the string creation below will
        // invalidate errno due to one more system calls.
        auto error = errno;
        throw std::runtime_error(
            "inotify_init1 failed, errno="s + std::strerror(error));
    }

    inotifyFD = std::make_unique<CustomFD>(fd);
    auto& inotFD = *(inotifyFD.get());

    wd = inotify_add_watch(inotFD(), CORE_FILE_DIR, IN_CLOSE_WRITE);
    if (-1 == wd)
    {
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

    namespace fs = std::experimental::filesystem;
    std::vector<fs::path> corePaths;

    while (offset < bytes)
    {
        auto event = reinterpret_cast<inotify_event*>(&buffer[offset]);
        if ((event->mask & IN_CLOSE_WRITE) && !(event->mask & IN_ISDIR))
        {
            corePaths.push_back(CORE_FILE_DIR + std::string(event->name));
        }

        offset += offsetof(inotify_event, name) + event->len;
    }

    //TODO Enable Dump collection function here

    return 0;
}

} // namespace dump
} // namespace phosphor
