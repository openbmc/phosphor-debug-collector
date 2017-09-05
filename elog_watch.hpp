#pragma once

#include <set>

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
using EId = uint32_t;
using ElogList = std::set<EId>;

/** @class Watch
 *  @brief Adds d-bus signal based watch for elog add and delete.
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

        /** @brief constructs watch for elog add and delete signals.
         *  @param[in] bus -  The Dbus bus object
         *  @param[in] intMgr - Dump internal Manager object
         */
        Watch(sdbusplus::bus::bus& bus, IMgr& iMgr);/*
            iMgr(iMgr),
            addMatch(
                bus,
                sdbusplus::bus::match::rules::interfacesAdded() +
                sdbusplus::bus::match::rules::path_namespace(
                    OBJ_LOGGING),
                std::bind(std::mem_fn(&Watch::addCallback),
                          this, std::placeholders::_1)),
            delMatch(
                bus,
                sdbusplus::bus::match::rules::interfacesRemoved() +
                sdbusplus::bus::match::rules::path_namespace(
                    OBJ_LOGGING),
                std::bind(std::mem_fn(&Watch::delCallback),
                          this, std::placeholders::_1))
        {
            //Do nothing
        }*/
    private:

        /** @brief Callback function for error log add.
         *  @details InternalError type error message initiates
         *           Internal error type dump request.
         *  @param[in] msg  - Data associated with subscribed signal
         */
        void addCallback(sdbusplus::message::message& msg);

        /** @brief Callback function for error log delete.
         *  @param[in] msg  - Data associated with subscribed signal
         */
        void delCallback(sdbusplus::message::message& msg);

        /** @brief get elog ID elog entry object string.
         *  @param[in] objectPath  - Object path.
         *  @return - elog id.
         */
        EId  getEid(const std::string& objectPath);

        /**  @brief Dump internal Manager object. */
        IMgr& iMgr;

        /** @brief sdbusplus signal match for elog add */
        sdbusplus::bus::match_t addMatch;

        /** @brief sdbusplus signal match for elog delete */
        sdbusplus::bus::match_t delMatch;

        /** @brief List of elog, which already requested dump */
        ElogList elogList;
};

}//namespace elog
}//namespace dump
}//namespace phosphor
