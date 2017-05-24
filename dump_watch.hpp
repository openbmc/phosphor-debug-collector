#pragma once

#include <map>
#include <memory>
#include <systemd/sd-event.h>

namespace phosphor
{
namespace dump
{

/** @struct CustomFD
 *
 *  RAII wrapper for file descriptor.
 */
struct CustomFD
{
        CustomFD(const CustomFD&) = delete;
        CustomFD& operator=(const CustomFD&) = delete;
        CustomFD(CustomFD&&) = delete;
        CustomFD& operator=(CustomFD&&) = delete;

        CustomFD(int fd) :
            fd(fd) {}

        ~CustomFD();

        int operator()() const
        {
            return fd;
        }

    private:
        int fd = -1;
};

/** @class Watch
 *
 *  @brief Adds inotify watch on core file directories.
 *
 *  The inotify watch is hooked up with sd-event, so that on call back,
 *  appropriate actions related to collect core files.
 */
class Watch
{
    public:
        /** @brief ctor - hook inotify watch with sd-event
         *
         *  @param[in] loop - sd-event object
         */
        Watch(sd_event* loop);

        Watch(const Watch&) = delete;
        Watch& operator=(const Watch&) = delete;
        Watch(Watch&&) = default;
        Watch& operator=(Watch&&) = default;

        /* @brief dtor - remove inotify watch and close fd's */
        ~Watch();

    private:
        /** @brief sd-event callback
         *
         *  @param[in] s - event source, floating (unused) in our case
         *  @param[in] fd - inotify fd
         *  @param[in] revents - events that matched for fd
         *  @param[in] userdata - pointer to Watch object
         *  @returns 0 on success, -1 on fail
         */
        static int callback(sd_event_source* s,
                            int fd,
                            uint32_t revents,
                            void* userdata);

        /** @brief core file directory watch descriptor */
        int wd = -1;

        /** @brief inotify file descriptor */
        std::unique_ptr<CustomFD> inotifyFD = nullptr;
};

} // namespace dump
} // namespace phosphor
