#include <stdexcept>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <sys/inotify.h>
#include <unistd.h>
#include "config.h"
#include "watch.hpp"
#include <experimental/filesystem>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include "elog-errors.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "dump_manager.hpp"

namespace phosphor
{
namespace dump
{
namespace report
{

using namespace std::string_literals;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
namespace fs = std::experimental::filesystem;

Watch::~Watch()
{
    if ((fd() >= 0) && (wd >= 0))
    {
        inotify_rm_watch(fd(), wd);
    }
}

Watch::Watch(sd_event* loop, Manager* mgr):
    fd(inotifyInit()),
    mgr(mgr)
{
    // Check if BMC DUMP DIR exists.
    fs::path dumpDirPath(BMC_DUMP_FILE_DIR);
    if (!fs::is_directory(dumpDirPath))
    {
        //TODO Add elog
        log<level::ERR>("Invalid BMC DUMP Path");
        return;
    }

    // TODO openbmc/openbmc#1795
    // Rebuild Dump Entry objects based  Dump Files in the flash during reboot.

    wd = inotify_add_watch(fd(), BMC_DUMP_FILE_DIR, (IN_CLOSE_WRITE | IN_DELETE));
    if (-1 == wd)
    {
        auto error = errno;
        log<level::ERR>("Error occurred during the inotify_add_watch call",
                        entry("ERRNO=%s", strerror(error)));
        elog<InternalFailure>();
    }

    auto rc = sd_event_add_io(loop,
                              nullptr,
                              fd(),
                              EPOLLIN,
                              callback,
                              this);
    if (0 > rc)
    {
        // Failed to add to event loop
        auto error = errno;
        log<level::ERR>("Error occurred during the sd_event_add_io call",
                        entry("ERRNO=%s", strerror(error)));
        elog<InternalFailure>();
    }
}

int Watch::inotifyInit()
{
    auto fd = inotify_init1(IN_NONBLOCK);

    if (-1 == fd)
    {
        auto error = errno;
        log<level::ERR>("Error occurred during the inotify_init1",
                        entry("ERRNO=%s", strerror(error)));
        elog<InternalFailure>();
    }

    return fd;
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
                        entry("ERRNO=%s", strerror(error)));
        //report<InternalFailure>();
        return 0;
    }

    auto offset = 0;

    std::vector<fs::path> dumpPaths;
    std::vector<fs::path> delPaths;

    while (offset < bytes)
    {
        auto event = reinterpret_cast<inotify_event*>(&buffer[offset]);
        if ((event->mask & IN_CLOSE_WRITE) && !(event->mask & IN_ISDIR))
        {
            dumpPaths.emplace_back(fs::path(BMC_DUMP_FILE_DIR) / event->name);
        }
        else if ((event->mask & IN_DELETE) && !(event->mask & IN_ISDIR))
        {
            delPaths.emplace_back(fs::path(BMC_DUMP_FILE_DIR) / event->name);
        }
        offset += offsetof(inotify_event, name) + event->len;
    }

    // Generate new BMC Dump d-bus entry incase valid Dump files
    if (!dumpPaths.empty())
    {
        for (auto& i : dumpPaths)
        {
            static_cast<Watch*>(userdata)->mgr->createEntry(i.string().c_str());
        }
    }

    if (!delPaths.empty())
    {
        // TODO openbmc/openbmc#1795
        // Remove Dump d-bus entry objects for file deletion in Dump partiotion.
    }
    return 0;
}

} // namespace report
} // namespace dump
} // namespace phosphor
