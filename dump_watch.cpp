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
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <elog-errors.hpp>
#include <xyz/openbmc_project/Dump/Monitor/error.hpp>

namespace phosphor
{
namespace dump
{

using namespace std::string_literals;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Dump::Monitor::Error;
namespace fs = std::experimental::filesystem;

CustomFd::~CustomFd()
{
    if (-1 != fd)
    {
        close(fd);
    }
}

Watch::~Watch()
{
    if (-1 != Fd())
    {
        inotify_rm_watch(Fd(), wd);
    }
}

Watch::Watch(sd_event* loop):
    Fd(inotifyInit())
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
        //TODO Issue #1510 Enable Dump collection function here.
    }

    wd = inotify_add_watch(Fd(), CORE_FILE_DIR, IN_CLOSE_WRITE);
    if (-1 == wd)
    {
        auto error = errno;
        namespace metadata = xyz::openbmc_project::Dump::Monitor ;
        elog<InternalFailure>(metadata::InternalFailure::FAIL("inotify_add_watch"),
                              metadata::InternalFailure::ERRNO(strerror(error)));
    }

    auto rc = sd_event_add_io(loop,
                              nullptr,
                              Fd(),
                              EPOLLIN,
                              callback,
                              this);
    if (0 > rc)
    {
        // Failed to add to event loop
        auto error = errno;
        namespace metadata = xyz::openbmc_project::Dump::Monitor ;
        elog<InternalFailure>(metadata::InternalFailure::FAIL("sd_event_add_io"),
                              metadata::InternalFailure::ERRNO(strerror(error)));
    }
}

int Watch::inotifyInit()
{
    auto fd = inotify_init1(IN_NONBLOCK);

    if (-1 == fd)
    {
        auto error = errno;
        namespace metadata = xyz::openbmc_project::Dump::Monitor;
        elog<InternalFailure>(metadata::InternalFailure::FAIL("inotify_init1"),
                              metadata::InternalFailure::ERRNO(strerror(error)));
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

    constexpr auto maxBytes = 1024;
    uint8_t buffer[maxBytes];
    auto bytes = read(fd, buffer, maxBytes);
    if (0 > bytes)
    {
        //Failed to read inotify event
        //Report error and return
        auto error = errno;
        namespace metadata = xyz::openbmc_project::Dump::Monitor;
        report<InternalFailure>(metadata::InternalFailure::FAIL("read"),
                                metadata::InternalFailure::ERRNO(strerror(error)));
        return 0;
    }

    auto offset = 0;

    std::vector<fs::path> corePaths;

    while (offset < bytes)
    {
        auto event = reinterpret_cast<inotify_event*>(&buffer[offset]);
        if ((event->mask & IN_CLOSE_WRITE) && !(event->mask & IN_ISDIR))
        {
            fs::path p(CORE_FILE_DIR);
            corePaths.emplace_back(p /= event->name);
        }

        offset += offsetof(inotify_event, name) + event->len;
    }

    // Generate new BMC Dump( Core dump Type) incase valid cores
    if (!corePaths.empty())
    {
        //TODO Issue #1510 Enable Dump collection function here
    }
    return 0;
}

} // namespace dump
} // namespace phosphor
