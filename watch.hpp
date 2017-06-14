#pragma once

#include <unistd.h>
#include <systemd/sd-event.h>
#include "dump_utils.hpp"
#include "dump_manager.hpp"

namespace phosphor
{
namespace dump
{
namespace notify
{

/** @class Watch
 *
 *  @brief Adds inotify watch on dump file directory.
 *
 *  The inotify watch is hooked up with sd-event, so that on call back,
 *  appropriate actions are taken to update dump entry d-bus objects.
 */
class Watch
{
    public:
        /** @brief ctor - hook inotify watch with sd-event
         *
         *  @param[in] event - sd-event object
         *  @param[in] entryCallback - The callback function for creating
         *                             dump entry
         */
        Watch( const EventPtr& event,
              std::function<void(fs::path&)> entryCallback);

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
         *
         *  @returns 0 on success, -1 on fail
         */
        static int callback(sd_event_source* s,
                            int fd,
                            uint32_t revents,
                            void* userdata);

        /**  initialize an inotify instance and returns file descriptor */
        int inotifyInit();

        /** @brief dump file directory watch descriptor */
        int wd = -1;

        /** @brief file descriptor manager */
        CustomFd fd;

        /** @brief The callback function for creating the dump entry. */
        std::function<void(fs::path&)> entryCallback;
};

} // namespace notify
} // namespace dump
} // namespace phosphor
