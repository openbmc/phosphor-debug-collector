#pragma once

#include "config.h"

#include "dump_utils.hpp"
#include "watch.hpp"

#include <sdeventplus/utility/timer.hpp>

#include <map>
#include <queue>
#include <string>
#include <vector>

namespace phosphor
{
namespace dump
{
namespace core
{
using Watch = phosphor::dump::inotify::Watch;
using UserMap = phosphor::dump::inotify::UserMap;

/** workaround: Watches for IN_CLOSE_WRITE event for the
 *  jffs filesystem based systemd-coredump core path
 *  Refer openbmc/issues/#2287 for more details.
 *
 *  JFFS_CORE_FILE_WORKAROUND will be enabled for jffs and
 *  for other file system it will be disabled.
 */
#ifdef JFFS_CORE_FILE_WORKAROUND
static constexpr auto coreFileEvent = IN_CLOSE_WRITE;
#else
static constexpr auto coreFileEvent = IN_CREATE;
#endif

/** @class Manager
 *  @brief OpenBMC Core manager implementation.
 */
class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to create core watch object.
     *  @param[in] event - Dump manager sd_event loop.
     */
    Manager(const EventPtr& event) :
        eventLoop(event.get()),
        coreWatch(eventLoop, IN_NONBLOCK, coreFileEvent, EPOLLIN, CORE_FILE_DIR,
                  std::bind(std::mem_fn(
                                &phosphor::dump::core::Manager::watchCallback),
                            this, std::placeholders::_1)),
        coreDumpTimer(
            event.get(),
            std::bind(std::mem_fn(&Manager::processCoreDumpQueue), this),
            std::chrono::seconds(300))
    {
        // Initially disabling the timer. We will enable only during failure.
        coreDumpTimer.setEnabled(false);
    }

    /** @brief A queue of core dump requests. */
    static std::queue<std::vector<std::string>> coreDumpQueue;

  private:
    /** @brief Helper function for initiating dump request using
     *         createDump D-Bus interface.
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

    /** @brief Timer object */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> coreDumpTimer;

    /** @brief Function to process all core dump requests
     */
    void processCoreDumpQueue();
};

} // namespace core
} // namespace dump
} // namespace phosphor
