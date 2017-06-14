#include <sys/inotify.h>
#include <unistd.h>
#include <experimental/filesystem>

#include <xyz/openbmc_project/Dump/Manager/error.hpp>
#include <phosphor-logging/elog-errors.hpp>

#include "elog-errors.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "config.h"
#include "watch.hpp"
#include "dump_manager.hpp"

namespace phosphor
{
namespace dump
{
namespace notify
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace sdbusplus::xyz::openbmc_project::Dump::Manager::Error;

Watch::~Watch()
{
    if ((fd() >= 0) && (wd >= 0))
    {
        inotify_rm_watch(fd(), wd);
    }
}

Watch::Watch(const EventPtr& event,
             std::function<void(fs::path&)> entryCallback):
    fd(inotifyInit()),
    entryCallback(entryCallback)
{
    // Check if BMC DUMP DIR exists.
    fs::path dumpDirPath(BMC_DUMP_FILE_DIR);
    if (!fs::is_directory(dumpDirPath))
    {
        namespace metadata = xyz::openbmc_project::Dump::Manager;
        elog<InvalidDumpPath>(metadata::InvalidDumpPath::PATH(BMC_DUMP_FILE_DIR));
    }

    // TODO openbmc/openbmc#1795
    // Rebuild entry objects during dump manager daemon start
    // based on the available dump files in the persistent storage
    // Add File deletion event to remove dump entry objects
    // during dump file deletion.

    wd = inotify_add_watch(fd(), BMC_DUMP_FILE_DIR, IN_CLOSE_WRITE);
    if (-1 == wd)
    {
        auto error = errno;
        log<level::ERR>("Error occurred during the inotify_add_watch call",
                        entry("ERRNO=%s", strerror(error)));
        elog<InternalFailure>();
    }

    auto rc = sd_event_add_io(event.get(),
                              nullptr,
                              fd(),
                              EPOLLIN,
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

    std::vector<fs::path> dumpPaths;

    while (offset < bytes)
    {
        auto event = reinterpret_cast<inotify_event*>(&buffer[offset]);
        if ((event->mask & IN_CLOSE_WRITE) && !(event->mask & IN_ISDIR))
        {
            dumpPaths.emplace_back(fs::path(BMC_DUMP_FILE_DIR) / event->name);
        }
        // TODO openbmc/openbmc#1795
        // Handle File deletion event.
        offset += offsetof(inotify_event, name) + event->len;
    }

    // Generate new BMC Dump d-bus entry incase valid Dump files
    if (!dumpPaths.empty())
    {
        for (auto& file : dumpPaths)
        {
            static_cast<Watch*>(userdata)->entryCallback(file);
        }
    }

    return 0;
}

} // namespace notify
} // namespace dump
} // namespace phosphor
