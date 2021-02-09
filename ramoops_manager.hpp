#pragma once

#include "config.h"

#include "watch.hpp"

namespace phosphor
{
namespace dump
{
namespace ramoops
{
using Watch = phosphor::dump::inotify::Watch;
using UserMap = phosphor::dump::inotify::UserMap;

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

    /** @brief Constructor to create ramoops watch object.
     *  @param[in] event    - Dump manager sd_event loop.
     *  @param[in] filePath - Path where the ramoops are stored.
     */
    Manager(const EventPtr& event, const char* filePath) :
        eventLoop(event.get()),
        RamoopsWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            filePath,
            std::bind(
                std::mem_fn(&phosphor::dump::ramoops::Manager::watchCallback),
                this, std::placeholders::_1))
    {
    }

  private:
    /** @brief Helper function for initiating dump request using
     *         D-bus internal create interface.
     *  @param [in] files - Core files list
     */
    void createHelper(const std::set<std::string>& files);

    /** @brief Implementation of ramoops watch call back
     * @param [in] fileInfo - map of file info  path:event
     */
    void watchCallback(const UserMap& fileInfo);

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief ramoops watch object */
    Watch RamoopsWatch;
};

} // namespace ramoops
} // namespace dump
} // namespace phosphor
