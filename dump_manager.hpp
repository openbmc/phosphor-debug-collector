#pragma once

#include "dump_entry.hpp"
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
    friend class Entry;

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
        bus(bus), lastEntryId(0), baseEntryPath(baseEntryPath)
    {}

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    virtual void restore() = 0;

    /** @brief Create a  Dump Entry Object
     *  @param[in] id - Id of the dump
     *  @param[in] objPath - Object path to attach to
     *  @param[in] timeStamp - Dump creation timestamp
     *             since the epoch.
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     *  @param[in] status - status of the dump.
     *  @param[in] parent - The dump entry's parent.
     */
    virtual void createEntry(const uint32_t id, const std::string objPath,
                             const uint64_t ms, uint64_t fileSize,
                             const std::filesystem::path& file,
                             phosphor::dump::OperationStatus status,
                             std::string originatorId,
                             originatorTypes originatorType) = 0;

    std::optional<std::reference_wrapper<std::unique_ptr<Entry>>>
        getEntryById(uint32_t id)
    {
        auto it = entries.find(id);
        if (it != entries.end())
        {
            return std::make_optional(std::ref(it->second));
        }
        else
        {
            return std::nullopt;
        }
    }

    /** @brief Gets the object path for an entry with a specified ID.
     *
     * @param[in] id - The ID of the entry.
     * @return The object path for the entry.
     */
    std::filesystem::path getEntryObjectPath(uint32_t id) const
    {
        return std::filesystem::path(baseEntryPath) / std::to_string(id);
    }

    uint32_t getLastEntryId()
    {
        return lastEntryId;
    }

    void updateLastEntryId(uint32_t id)
    {
        lastEntryId = id;
    }

    /**
     * @brief Retrieves a pointer to the Entry with the lowest id.
     * This function finds the Entry with the lowest id and returns a pointer to
     * it.
     *
     * @return std::optional<Entry> Pointer to the Entry with the lowest id.
     * Returns std::nullopt if the entries are empty.
     */
    std::optional<Entry*> getEntryWithLowestId()
    {
        if (entries.empty())
        {
            // Return std::nullopt if entries are empty
            return std::nullopt;
        }

        // Find the entry with the lowest id
        auto entryIter = min_element(entries.begin(), entries.end(),
                                     [](const auto& l, const auto& r) {
            return l.first < r.first;
        });

        // Return a pointer to the Entry
        return entryIter->second.get();
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
    std::map<uint32_t, std::unique_ptr<Entry>> entries;

    /** @brief Id of the last Dump entry */
    uint32_t lastEntryId;

    /** @bried base object path for the entry object */
    std::string baseEntryPath;
};

} // namespace dump
} // namespace phosphor
