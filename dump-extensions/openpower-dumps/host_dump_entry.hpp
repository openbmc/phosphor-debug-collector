#pragma once

#include "dump_entry.hpp"
#include "dump_manager.hpp"
#include "op_dump_consts.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>

namespace openpower::dump::host
{

constexpr uint32_t CLASS_SERIALIZATION_VERSION = 1;

/** @class Entry
 *  @brief Base class for Host Dump Entry implementation.
 *  @details A base implementation for dump entries that provides common
 *  functionality.
 */
template <typename Derived>
class Entry : virtual public phosphor::dump::Entry
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    ~Entry() = default;

    /** @brief Constructor for the Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] dumpId - Dump id.
     *  @param[in] timeStamp - Dump creation timestamp since the epoch.
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     *  @param[in] status - Status of the dump.
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     *  @param[in] parent - The dump entry's parent.
     *  @param[in] transportId - Transport identifier for the dump type.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize,
          phosphor::dump::OperationStatus status, std::string originatorId,
          phosphor::dump::originatorTypes originatorType,
          phosphor::dump::Manager& parent, uint32_t transportId);

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump.
     */
    void initiateOffload(std::string uri);

    /** @brief Method to update an existing dump entry
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     */
    void update(uint64_t timeStamp, uint64_t dumpSize, uint32_t sourceId);

    /**
     * @brief Delete host system dump and it entry dbus object
     */
    void delete_();

    /** @brief Serialize the dump entry
     */
    void serialize();

    /** @brief Deserialize the dump entry
     *  @param[in] dumpPath - Path to the directory from where the entry will be
     * deserialized
     */
    void deserialize(const std::filesystem::path& dumpPath);

  protected:
    void removeSerializedEntry();
    uint32_t transportId;

    /**
     * @brief Gets the source dump ID.
     * @return The source dump ID.
     */
    auto getSourceDumpId() const
    {
        return static_cast<const Derived*>(this)->sourceDumpId();
    }

    /**
     * @brief Sets the source dump ID.
     * @param[in] id The source dump ID to set.
     */
    void setSourceDumpId(const uint32_t id)
    {
        static_cast<Derived*>(this)->sourceDumpId(id);
    }

    /**
     * @brief Gets the VSP (Vital Product Data) string.
     * @return The VSP string.
     */
    auto getVspString() const
    {
        return static_cast<const Derived*>(this)->vspString();
    }

    /**
     * @brief Sets the VSP (Vital Product Data) string.
     * @param[in] vspStr The VSP string to set.
     */
    void setVspString(const std::string& vspStr)
    {
        static_cast<Derived*>(this)->vspString(vspStr);
    }

    /**
     * @brief Gets the user challenge string.
     * @return The user challenge string.
     */
    auto getUserChallenge() const
    {
        return static_cast<const Derived*>(this)->userChallenge();
    }

    /**
     * @brief Sets the user challenge string.
     * @param[in] usrChallenge The user challenge string to set.
     */
    void setUserChallenge(const std::string& usrChallenge)
    {
        static_cast<Derived*>(this)->userChallenge(usrChallenge);
    }

    /**
     * @brief Gets the dump request status.
     * @return The dump request status.
     */
    auto getDumpRequestStatus() const
    {
        return static_cast<const Derived*>(this)->dumpRequestStatus();
    }

    /**
     * @brief Sets the dump request status.
     * @param[in] status The dump request status to set.
     */
    void setDumpRequestStatus(const auto& status)
    {
        static_cast<Derived*>(this)->setDumpRequestStatusImpl(status);
    }
};

} // namespace openpower::dump::host
