#pragma once

#include "config.h"

#include "watch.hpp"

namespace phosphor
{
namespace dump
{
namespace kernel
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

    /** @brief Constructor to create kernel watch object.
     *  @param[in] event - Dump manager sd_event loop.
     */
    Manager(const EventPtr& event) :
        eventLoop(event.get()),
        kernelWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            SYSTEMD_PSTORE_PATH,
            std::bind(
                std::mem_fn(&phosphor::dump::kernel::Manager::watchCallback),
                this, std::placeholders::_1))
    {
    }

  private:
    /** @brief Helper function for initiating dump request using
     *         D-bus internal create interface.
     *  @param [in] files - Core files list
     */
    void createHelper(const std::vector<std::string>& files);

    /** @brief Implementation of kernel watch call back
     * @param [in] fileInfo - map of file info  path:event
     */
    void watchCallback(const UserMap& fileInfo);

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Kernel watch object */
    Watch kernelWatch;
};

} // namespace kernel
} // namespace dump
} // namespace phosphor
