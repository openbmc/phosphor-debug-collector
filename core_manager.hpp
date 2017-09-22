#pragma once

#include <map>

#include "dump_utils.hpp"
#include "watch.hpp"
#include "config.h"

namespace phosphor
{
namespace dump
{
namespace core
{
using Watch = phosphor::dump::inotify::Watch;
using UserMap = phosphor::dump::inotify::UserMap;

/** workaround: Watches for IN_CREATE event for the 
 *  ubi filesystem based systemd-coredump core path
 *  Refer openbmc/issues/#2287 for more details.
 */
#ifdef UBI_CORE_FILE_WORKAROUND
   static constexpr auto CORE_FILE_EVENT = IN_CREATE;
#else
   static constexpr auto CORE_FILE_EVENT = IN_CLOSE_WRITE;
#endif

/** @class Manager
 *  @brief OpenBMC Core manager implementation.
 */
class Manager
{
    public:
        Manager() = delete;
        Manager(const Manager&) = default;
        Manager& operator=(const Manager&) = delete;
        Manager(Manager&&) = delete;
        Manager& operator=(Manager&&) = delete;
        virtual ~Manager() = default;

        /** @brief Constructor to create core watch object.
         *  @param[in] event - Dump manager sd_event loop.
         */
        Manager(const EventPtr& event) :
            eventLoop(event.get()),
            coreWatch(eventLoop,
                      IN_NONBLOCK,
                      CORE_FILE_EVENT,
                      EPOLLIN,
                      CORE_FILE_DIR,
                      std::bind(
                          std::mem_fn(
                              &phosphor::dump::core::Manager::watchCallback),
                          this, std::placeholders::_1))
        {}

    private:
        /** @brief Helper function for initiating dump request using
         *         D-bus internal create interface.
         *  @param [in] files - Core files list
         */
        void createHelper(const std::vector<std::string>& files);


        /** @brief Implementation of core watch call back
         * @param [in] fileInfo - map of file info  path:event
         */
        void watchCallback(const UserMap& fileInfo);

        /** @brief sdbusplus Dump event loop */
        EventPtr eventLoop;

        /** @brief Core watch object */
        Watch coreWatch;
};

} // namepsace core
} // namespace dump
} // namespace phosphor
