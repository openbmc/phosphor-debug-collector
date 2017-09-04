#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include "config.h"

#include "dump_manager.hpp"

namespace phosphor
{
namespace dump
{
namespace elog
{

using IMgr = phosphor::dump::internal::Manager;

/** @class Watch
 *  @brief Adds d-bus signal based watch for elog commit.
 *  @details This implements methods for watching for InternalFailure
 *  type error message and call appropriate function to initiate dump
 */
class Watch
{
    public:
        Watch() = delete;
        ~Watch() = default;
        Watch(const Watch&) = delete;
        Watch& operator=(const Watch&) = delete;
        Watch(Watch&&) = default;
        Watch& operator=(Watch&&) = default;

        /** @brief constructs watch for elog commits.
         *  @param[in] bus -  The Dbus bus object
         *  @param[in] intMgr - Dump internal Manager object
         */
        Watch(sdbusplus::bus::bus& bus, IMgr& iMgr):
            iMgr(iMgr),
            matchCreated(
                bus,
                sdbusplus::bus::match::rules::interfacesAdded() +
                sdbusplus::bus::match::rules::path_namespace(
                    "/xyz/openbmc_project/logging"),
                std::bind(std::mem_fn(&Watch::callback),
                          this, std::placeholders::_1))
        {
            //Do nothing
        }
    private:

        /** @brief Callback function for error log commit.
         *  @details InternalError type error message initiates
         *           Internal error type dump request.
         *  @param[in] msg  - Data associated with subscribed signal
         */
        void callback(sdbusplus::message::message& msg);

        /**  @brief Dump internal Manager object. */
        IMgr& iMgr;

        /** @brief sdbusplus signal match for elog commit */
        sdbusplus::bus::match_t matchCreated;
};

}//namespace elog
}//namespace dump
}//namespace phosphor
