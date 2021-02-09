#pragma once

#include "config.h"

#include "dump_utils.hpp"
#include "file_watch_manager.hpp"
#include "watch.hpp"

#include <map>

namespace phosphor
{
namespace dump
{
namespace core
{

/** @class Manager
 *  @brief OpenBMC Core manager implementation.
 */
class Manager : public phosphor::dump::filewatch::Manager
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
        phosphor::dump::filewatch::Manager(
            event, CORE_FILE_DIR,
            "xyz.openbmc_project.Dump.Internal.Create.Type.ApplicationCored")
    {
    }

  private:
    /** @brief Implementation of core watch call back
     * @param [in] fileInfo - map of file info  path:event
     */
    void watchCallback(const filewatch::UserMap& fileInfo);
};

} // namespace core
} // namespace dump
} // namespace phosphor
