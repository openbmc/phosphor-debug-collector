#pragma once

#include "dump_manager_bmc.hpp"
#include "watch.hpp"

namespace phosphor
{
namespace dump
{
namespace ramoops
{

using IMgr = phosphor::dump::bmc::internal::Manager;
using UserMap = phosphor::dump::inotify::UserMap;
using Watch = phosphor::dump::inotify::Watch;

/** @class RamoopsWatch
 *  @brief Adds D-Bus signal based watch for ramoops.
 */
class RamoopsWatch
{
  public:
    RamoopsWatch() = delete;
    ~RamoopsWatch() = default;
    RamoopsWatch(const RamoopsWatch&) = delete;
    RamoopsWatch& operator=(const RamoopsWatch&) = delete;
    RamoopsWatch(RamoopsWatch&&) = default;
    RamoopsWatch& operator=(RamoopsWatch&&) = default;

    /** @brief constructs watch for ramoops.
     *  @param[in] intMgr   - Dump internal Manager object
     *  @param[in] filePath - Path where the ramoops are stored.
     */
    RamoopsWatch(const EventPtr& event, IMgr& iMgr, const char* filePath) :
        eventLoop(event.get()), iMgr(iMgr),
        ramoopsWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            filePath,
            std::bind(
                std::mem_fn(
                    &phosphor::dump::ramoops::RamoopsWatch::watchCallback),
                this, std::placeholders::_1)),
        pstoreDir(filePath)
    {
        // Pick up files if they exist already when the application starts
        collectRamoopsData();
    }

  private:
    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /**  @brief Dump internal Manager object. */
    IMgr& iMgr;

    /** @brief Dump main watch object */
    Watch ramoopsWatch;

    /** @brief Path to the pstore file*/
    std::string pstoreDir;

    /** @brief Collect ramoops data into BMC dump
     */
    void collectRamoopsData();

    /** @brief Implementation of core watch call back
     * @param [in] fileInfo - map of file info
     */
    void watchCallback(const UserMap& fileInfo);
};

} // namespace ramoops
} // namespace dump
} // namespace phosphor
