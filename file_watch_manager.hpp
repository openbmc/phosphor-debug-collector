#pragma once

#include "config.h"

#include "dump_utils.hpp"
#include "watch.hpp"

#include <map>

namespace phosphor
{
namespace dump
{
namespace filewatch
{
using Watch = phosphor::dump::inotify::Watch;
using UserMap = phosphor::dump::inotify::UserMap;

/** workaround: Watches for IN_CLOSE_WRITE event for the
 *  jffs filesystem based systemd-coredump core or other file path
 *  Refer openbmc/issues/#2287 for more details.
 *
 *  JFFS_CORE_FILE_WORKAROUND will be enabled for jffs and
 *  for other file system it will be disabled.
 */
#ifdef JFFS_CORE_FILE_WORKAROUND
static constexpr auto fileEvent = IN_CLOSE_WRITE;
#else
static constexpr auto fileEvent = IN_CREATE;
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
    Manager(const EventPtr& event, const std::string watchDir,
            std::string dumpType) :
        eventLoop(event.get()),
        fileWatch(
            eventLoop, IN_NONBLOCK, fileEvent, EPOLLIN, watchDir,
            std::bind(
                std::mem_fn(&phosphor::dump::filewatch::Manager::watchCallback),
                this, std::placeholders::_1)),
        dumpType(dumpType)
    {
    }

  protected:
    /** @brief Helper function for initiating dump request using
     *         D-bus internal create interface.
     *  @param [in] files - Core files list
     */
    void createHelper(const std::vector<std::string>& files);

    /** @brief Implementation of core watch call back
     * @param [in] fileInfo - map of file info  path:event
     */
    virtual void watchCallback(const UserMap& fileInfo) = 0;

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Core watch object */
    Watch fileWatch;

    /** @brief type of the dump */
    std::string dumpType;
};

} // namespace filewatch
} // namespace dump
} // namespace phosphor
