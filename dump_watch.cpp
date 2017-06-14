#include <sys/inotify.h>
#include <experimental/filesystem>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Dump/Monitor/error.hpp>

#include "config.h"
#include "dump_watch.hpp"
#include "elog-errors.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

namespace phosphor
{
namespace dump
{
namespace inotify
{

using namespace std::string_literals;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Dump::Monitor::Error;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
namespace fs = std::experimental::filesystem;

Watch::~Watch()
{
    if ((fd() >= 0) && (wd >= 0))
    {
        inotify_rm_watch(fd(), wd);
    }
}

Watch::Watch(sd_event* loop):
    fd(inotifyInit())
{
    // Check if CORE DIR exists.
    fs::path coreDirPath(CORE_FILE_DIR);
    if (!fs::is_directory(coreDirPath))
    {
        namespace metadata = xyz::openbmc_project::Dump::Monitor;
        elog<InvalidCorePath>(metadata::InvalidCorePath::PATH(CORE_FILE_DIR));
    }

    //Check for existing coredumps, Dump manager should handle this before
    //starting the core monitoring.
    //This is to handle coredumps created prior to Dump applications start,
    //or missing coredump reporting due to Dump application crashs.
    if (!fs::is_empty(coreDirPath))
    {
        //TODO openbmc/openbmc#1510 Enable Dump collection function here.
    }

    wd = inotify_add_watch(fd(), CORE_FILE_DIR, IN_CLOSE_WRITE);
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
        report<InternalFailure>();
        return 0;
    }

    auto offset = 0;

    std::vector<fs::path> corePaths;

    while (offset < bytes)
    {
        auto event = reinterpret_cast<inotify_event*>(&buffer[offset]);
        if ((event->mask & IN_CLOSE_WRITE) && !(event->mask & IN_ISDIR))
        {
            corePaths.emplace_back(fs::path(CORE_FILE_DIR) / event->name);
        }

        offset += offsetof(inotify_event, name) + event->len;
    }

    // Generate new BMC Dump( Core dump Type) incase valid cores
    if (!corePaths.empty())
    {
        //TODO openbmc/openbmc#1510 Enable Dump collection function here
    }
    return 0;
}

} // namespace inotify
} // namespace dump
} // namespace phosphor
