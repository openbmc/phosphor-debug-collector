#pragma once

#include "base_dump_entry.hpp"
#include "xyz/openbmc_project/Collection/DeleteAll/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#define CREATE_DUMP_MAX_PARAMS 2

namespace phosphor
{
namespace dump
{

using DumpCreateParams =
    std::map<std::string, std::variant<std::string, uint64_t>>;
using Iface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Collection::server::DeleteAll>;

/** @class Manager
 *  @brief Dump  manager base class.
 *  @details A concrete implementation for the
 *  xyz::openbmc_project::Collection::server::DeleteAll.
 */
class Manager : public Iface
{
    friend class BaseEntry;

  public:
    Manager() = delete;
    Manager(const Manager&) = default;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] event - Dump manager sd_event loop.
     *  @param[in] path - Path to attach at.
     *  @param[in] baseEntryPath - Base path of the dump entry.
     */
    Manager(sdbusplus::bus_t& bus, const char* path,
            const std::string& baseEntryPath) :
        Iface(bus, path, Iface::action::defer_emit),
        bus(bus), baseEntryPath(baseEntryPath), lastEntryId(0)
    {}

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    virtual void restore() = 0;

    uint16_t getLastEntryId()
    {
        return lastEntryId;
    }

    void setLastEntryId(uint16_t id)
    {
        lastEntryId = id;
    }

    uint32_t currentEntryId(uint16_t mask = 0)
    {
        // Shift mask to upper 16 bits and combine with lastEntryId
        uint32_t id = ((static_cast<uint32_t>(mask) << 16) | lastEntryId);

        return id;
    }

    uint32_t incrementLastEntryId(uint16_t mask = 0)
    {
        // Increment lastEntryId for the next entry
        lastEntryId++;

        // Shift mask to upper 16 bits and combine with lastEntryId
        uint32_t nextId = ((static_cast<uint32_t>(mask) << 16) | lastEntryId);

        return nextId;
    }

    /** @brief Returns a specific entry based on the ID
     *
     * @param[in] id - unique identifier of the entry
     *
     * @return BaseEntry* - pointer to the requested entry
     *
     */
    inline BaseEntry* getEntry(uint32_t id)
    {
        auto it = entries.find(id);
        if (it == entries.end())
        {
            return nullptr;
        }
        return it->second.get();
    }

  protected:
    /** @brief Erase specified entry d-bus object
     *
     * @param[in] entryId - unique identifier of the entry
     */
    void erase(uint32_t entryId);

    /** @brief  Erase all BMC dump entries and  Delete all Dump files
     * from Permanent location
     *
     */
    void deleteAll() override;

    /** @brief sdbusplus DBus bus connection. */
    sdbusplus::bus_t& bus;

    /** @brief Dump Entry dbus objects map based on entry id */
    std::map<uint32_t, std::unique_ptr<BaseEntry>> entries;

    /** @bried base object path for the entry object */
    std::string baseEntryPath;

  private:
    /** @brief Last four digits of the last dump entry*/
    uint16_t lastEntryId;
};

} // namespace dump
} // namespace phosphor
