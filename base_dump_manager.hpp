#pragma once

#include "xyz/openbmc_project/Collection/DeleteAll/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace phosphor
{
namespace dump
{
using DumpCreateParams =
    std::map<std::string, std::variant<std::string, uint64_t>>;

using Iface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Collection::server::DeleteAll,
    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

/** @class BaseManager
 *  @brief Abstract base class for a Dump Manager.
 *
 *  @details This class provides a common interface for managing
 *  dumps. It is designed to be specialized by derived classes
 *  that implement the specific details of different types of dump
 *  managers. The BaseManager class itself inherits from the
 *  sdbusplus::xyz::openbmc_project::Collection::server::DeleteAll
 *  and the sdbusplus::xyz::openbmc_project::Dump::server::Create
 *  D-Bus interfaces, enabling it to provide the server-side
 *  functionality of these interfaces.
 */
class BaseManager : public Iface
{
    friend class BaseEntry;

  public:
    // Disallow copying and assignment operations.
    BaseManager() = delete;
    BaseManager(const BaseManager&) = default;
    BaseManager& operator=(const BaseManager&) = delete;
    BaseManager(BaseManager&&) = delete;
    BaseManager& operator=(BaseManager&&) = delete;

    virtual ~BaseManager() = default;

    /** @brief Constructs a new BaseManager object and places it onto the D-Bus
     * at the specified path.
     *  @param[in] bus - The D-Bus to attach to.
     *  @param[in] path - The D-Bus path to attach at.
     *
     *  Note: The emission of propertiesChanged signal is deferred until the
     *  derived classes complete their implementation.
     */
    BaseManager(sdbusplus::bus_t& bus, const char* path) :
        Iface(bus, path, Iface::action::defer_emit), bus(bus)
    {}

    /** @brief Constructs D-Bus objects from their persisted representations.
     *
     *  This function is meant to be overridden in derived classes.
     */
    virtual void restore() = 0;

  protected:
    /** @brief Removes a specific dump entry D-Bus object.
     *
     *  @param[in] entryId - Unique identifier of the dump entry to be erased.
     *
     *  This function is meant to be overridden in derived classes.
     */
    virtual void erase(uint32_t entryId) = 0;

    /** @brief sdbusplus DBus bus connection. */
    sdbusplus::bus_t& bus;
};

} // namespace dump
} // namespace phosphor
