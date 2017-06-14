#pragma once

#include <systemd/sd-event.h>
#include <unistd.h>
#include "dump_utils.hpp"

namespace phosphor
{
namespace dump
{
namespace inotify
{

/** @class Watch
 *
 *  @brief Adds inotify watch on core file directories.
 *
 *  The inotify watch is hooked up with sd-event, so that on call back,
 *  appropriate actions are taken to collect the core files.
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

        /**  initialize an inotify instance and returns file descriptor */
        int inotifyInit();

        /** @brief core file directory watch descriptor */
        int wd = -1;

        /** @brief file descriptor manager */
        CustomFd fd;
};

} // namespace inotify
} // namespace dump
} // namespace phosphor
