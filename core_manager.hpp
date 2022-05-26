#pragma once

#include "config.h"

#include "dump_utils.hpp"
#include "watch.hpp"

#include <sdeventplus/clock.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <vector>

namespace phosphor
{
namespace dump
{
namespace core
{

using ::phosphor::dump::inotify::UserMap;
using ::phosphor::dump::inotify::Watch;
using ::sdeventplus::ClockId::Monotonic;
using ::sdeventplus::utility::Timer;

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

static constexpr auto retryCount = 3;

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
        coreWatch(eventLoop, IN_NONBLOCK, coreFileEvent, EPOLLIN, CORE_FILE_DIR,
                  std::bind(std::mem_fn(
                                &phosphor::dump::core::Manager::watchCallback),
                            this, std::placeholders::_1)),
        retryTimer(event.get(),
                   std::bind(std::mem_fn(
                                 &phosphor::dump::core::Manager::retryCoreDump),
                             this),
                   retryTimeInSec)
    {
        retryTimer.setEnabled(false);
    }

  private:
    /** @brief Helper function for initiating dump request using
     *         D-bus internal create interface.
     */
    void createHelper();

    /**
     * @brief Helper method to retry core dump
     */
    void retryCoreDump();

    /** @brief Implementation of core watch call back
     * @param [in] fileInfo - map of file info  path:event
     */
    void watchCallback(const UserMap& fileInfo);

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Core watch object */
    Watch coreWatch;

    /** @brief Core files */
    std::vector<std::string> files;

    /** @brief Retry time to attempt core dump */
    const std::chrono::seconds retryTimeInSec{2};

    /** @brief Retry core dump */
    Timer<Monotonic> retryTimer;

    /** retry counter - to keep track of no of retries */
    uint16_t retryCounter = 0;
};

} // namespace core
} // namespace dump
} // namespace phosphor
