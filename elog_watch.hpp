#pragma once

#include "config.h"

#include "dump_manager_bmc.hpp"

#include <cereal/access.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

#include <filesystem>
#include <set>

namespace phosphor
{
namespace dump
{
namespace elog
{

using Mgr =  phosphor::dump::bmc::Manager;
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
     *  @param[in] mgr - Dump internal Manager object
     */
    Watch(sdbusplus::bus_t& bus, Mgr& mgr);

  private:
    friend class cereal::access;

    /** @brief Function required by Cereal to perform serialization.
     *  @tparam Archive - Cereal archive type (binary in our case).
     *  @param[in] a - reference to Cereal archive.
     *  @param[in] version - Class version that enables handling
     *                       a serialized data across code levels
     */
    template <class Archive>
    void serialize(Archive& a, const std::uint32_t version)
    {
        a(elogList);

        // TODO: openbmc/phosphor-debug-collector#1
        //      Split into load/save so that it enables
        //      version compare during serialization
    }

    /** @brief Callback function for error log add.
     *  @details InternalError type error message initiates
     *           Internal error type dump request.
     *  @param[in] msg  - Data associated with subscribed signal
     */
    void addCallback(sdbusplus::message_t& msg);

    /** @brief Callback function for error log delete.
     *  @param[in] msg  - Data associated with subscribed signal
     */
    void delCallback(sdbusplus::message_t& msg);

    /** @brief get elog ID from elog entry object string.
     *  @param[in] objectPath  - elog entry object path.
     *  @return - elog id.
     */
    inline EId getEid(const std::string& objectPath)
    {
        std::filesystem::path path(objectPath);
        return std::stoul(path.filename());
    }

    /**  @brief BMC Dump Manager object. */
    Mgr& mgr;

    /** @brief sdbusplus signal match for elog add */
    sdbusplus::bus::match_t addMatch;

    /** @brief sdbusplus signal match for elog delete */
    sdbusplus::bus::match_t delMatch;

    /** @brief List of elog ids, which have associated dumps created */
    ElogList elogList;
};

} // namespace elog
} // namespace dump
} // namespace phosphor
