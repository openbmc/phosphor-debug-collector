#pragma once

#include "config.h"

#include "dump_utils.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"

#include <map>

namespace phosphor
{
namespace dump
{
namespace filewatch
{
using Type =
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;
using Watch = phosphor::dump::inotify::Watch;
using UserMap = phosphor::dump::inotify::UserMap;

/** workaround: Watches for IN_CREATE event for the
 *  ubi filesystem based systemd-coredump core path
 *  Refer openbmc/issues/#2287 for more details.
 */
#ifdef UBI_CORE_FILE_WORKAROUND
static constexpr auto coreFileEvent = IN_CREATE;
#else
static constexpr auto coreFileEvent = IN_CLOSE_WRITE;
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
    Manager(const EventPtr& event, const std::string watchDir, Type dumpType) :
        eventLoop(event.get()),
        fileWatch(
            eventLoop, IN_NONBLOCK, coreFileEvent, EPOLLIN, watchDir,
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
    Type dumpType;
};

} // namespace filewatch
} // namespace dump
} // namespace phosphor
