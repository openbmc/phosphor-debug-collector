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
    // Check if CORE DIR exists.
    namespace fs = std::experimental::filesystem;
    fs::path coreDirPath(CORE_FILE_DIR);
    if (!fs::is_directory(coreDirPath))
    {
        //TODO Add phosphor log.
        throw std::runtime_error(
            "No Core Dir, COREDIR=" + std::string{CORE_FILE_DIR});
    }

    //Check for existing coredumps, and report to Dump if any. 
    //This is to handle coredumps created prior to Dump applications start,
    //or missing coredump reporting due to Dump application crashs.
    if (!fs::is_empty(coreDirPath))
    {
        //TODO Enable Dump collection function here
        //using std::experimental::filesystem::directory_iterator;
        //for (auto& dirEntry : directory_iterator(CORE_FILE_DIR))
        //{
        //    printf(" Files : %s\n", dirEntry.path().c_str()) ;
        //}
    }

    auto fd = inotify_init1(IN_NONBLOCK);

    if (-1 == fd)
    {
        //TODO Add phosphor log.
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
            corePaths.push_back(std::string(CORE_FILE_DIR)
                                + '/' + std::string(event->name));
        }

        offset += offsetof(inotify_event, name) + event->len;
    }

    // Generate new BMC Dump( Core dump Type) incase valid cores
    if (corePaths.size())
    {
        //TODO Enable Dump collection function here
        //for (auto& i : corePaths)
        //{
        //    printf("File List: %s \n", i.string().c_str());
        //}
    }
    return 0;
}

} // namespace dump
} // namespace phosphor
