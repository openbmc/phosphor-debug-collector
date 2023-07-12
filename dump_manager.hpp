#pragma once

#include "base_dump_manager.hpp"
#include "dump_entry.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#define CREATE_DUMP_MAX_PARAMS 2

namespace phosphor
{
namespace dump
{

/** @class Manager
 *  @brief Dump  manager base class.
 *  @details A concrete implementation for the
 *  xyz::openbmc_project::Collection::server::DeleteAll.
 */
class Manager : public BaseManager
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
        BaseManager(bus, path),
        lastEntryId(0), baseEntryPath(baseEntryPath)
    {}

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
    virtual void deleteAll() override;

    /** @brief Dump Entry dbus objects map based on entry id */
    std::map<uint32_t, std::unique_ptr<BaseEntry>> entries;

    /** @brief Id of the last Dump entry */
    uint32_t lastEntryId;

    /** @bried base object path for the entry object */
    std::string baseEntryPath;
};

} // namespace dump
} // namespace phosphor
