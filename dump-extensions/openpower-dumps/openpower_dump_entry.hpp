#pragma once

#include "dump_entry.hpp"

#include <com/ibm/Dump/Entry/Hardware/server.hpp>
#include <com/ibm/Dump/Entry/Hostboot/server.hpp>
#include <com/ibm/Dump/Entry/SBE/server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <concepts>
#include <filesystem>

namespace openpower::dump
{

using originatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

/** @class Entry
 *  @brief Represents a generic dump entry for the OpenPOWER dumps.
 */
class Entry : public virtual phosphor::dump::Entry
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    virtual ~Entry() = default;

    /** @brief Constructor for the generic Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to.
     *  @param[in] dumpId - Unique identifier for the dump.
     *  @param[in] timeStamp - Dump creation timestamp since the epoch.
     *  @param[in] fileSize - Size of the dump file in bytes.
     *  @param[in] file - Path to the dump file.
     *  @param[in] status - Current status of the dump.
     *  @param[in] originatorId - Identifier of the originator of the dump.
     *  @param[in] originatorType - Type of the originator.
     *  @param[in] eid - Error log identifier associated with the dump.
     *  @param[in] failingUnit - Identifier of the failing unit associated with
     * the dump.
     *  @param[in] parent - Reference to the managing dump manager.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t fileSize,
          const std::filesystem::path& file,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, phosphor::dump::Manager& parent) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, fileSize,
                              file, status, originatorId, originatorType,
                              parent)
    {}

    /** @brief Delete the dump and D-Bus object
     */
    void delete_() override;

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump
     */
    void initiateOffload(std::string) override;

    /** @brief Method to update an existing dump entry, once the dump creation
     *  is completed this function will be used to update the entry which got
     *  created during the dump request.
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] fileSize - Dump file size in bytes.
     *  @param[in] file - Name of dump file.
     */
    void update(uint64_t timeStamp, uint64_t fileSize,
                const std::filesystem::path& filePath)
    {
        elapsed(timeStamp);
        size(fileSize);
        // TODO: Handled dump failed case with #ibm-openbmc/2808
        status(OperationStatus::Completed);
        file = filePath;
        // TODO: serialization of this property will be handled with
        // #ibm-openbmc/2597
        completedTime(timeStamp);
    }
};

namespace hostboot
{

using HostbootIntf = sdbusplus::server::object_t<
    sdbusplus::com::ibm::Dump::Entry::server::Hostboot>;

/** @class Entry
 *  @brief Host Dump Entry implementation.
 *  @details A concrete implementation for the
 *  host dump type DBus API
 */
class Entry : public virtual openpower::dump::Entry, public virtual HostbootIntf
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    virtual ~Entry() = default;

    /** @brief Constructor for the Hostboot Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to.
     *  @param[in] dumpId - Unique identifier for the dump.
     *  @param[in] timeStamp - Dump creation timestamp since the epoch.
     *  @param[in] fileSize - Size of the dump file in bytes.
     *  @param[in] file - Path to the dump file.
     *  @param[in] status - Current status of the dump.
     *  @param[in] originatorId - Identifier of the originator of the dump.
     *  @param[in] originatorType - Type of the originator.
     *  @param[in] eid - Error log identifier associated with the dump.
     *  @param[in] failingUnit - Identifier of the failing unit associated with
     * the dump.
     *  @param[in] parent - Reference to the managing dump manager.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t fileSize,
          const std::filesystem::path& file,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, uint64_t eid,
          phosphor::dump::Manager& parent) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, fileSize,
                              file, status, originatorId, originatorType,
                              parent),
        openpower::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp,
                               fileSize, file, status, originatorId,
                               originatorType, parent),
        HostbootIntf(bus, objPath.c_str(), HostbootIntf::action::defer_emit)
    {
        errorLogId(eid);
        this->openpower::dump::hostboot::HostbootIntf::emit_object_added();
    }
};

} // namespace hostboot

namespace hardware
{

using HardwareIntf = sdbusplus::server::object_t<
    sdbusplus::com::ibm::Dump::Entry::server::Hardware>;

/** @class Entry
 *  @brief Hardware Dump Entry implementation.
 *  @details A concrete implementation for the hardware dump type DBus API.
 *           This class extends the general dump entry with hardware-specific
 *           attributes, such as error log ID and failing unit ID.
 */
class Entry : public virtual openpower::dump::Entry, public virtual HardwareIntf
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    virtual ~Entry() = default;

    /** @brief Constructor for the Hardware Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to.
     *  @param[in] dumpId - Unique identifier for the dump.
     *  @param[in] timeStamp - Dump creation timestamp since the epoch.
     *  @param[in] fileSize - Size of the dump file in bytes.
     *  @param[in] file - Path to the dump file.
     *  @param[in] status - Current status of the dump.
     *  @param[in] originatorId - Identifier of the originator of the dump.
     *  @param[in] originatorType - Type of the originator.
     *  @param[in] eid - Error log identifier associated with the dump.
     *  @param[in] failingUnit - Identifier of the failing unit associated with
     * the dump.
     *  @param[in] parent - Reference to the managing dump manager.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t fileSize,
          const std::filesystem::path& file,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, uint64_t eid, uint64_t failingUnit,
          phosphor::dump::Manager& parent) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, fileSize,
                              file, status, originatorId, originatorType,
                              parent),
        openpower::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp,
                               fileSize, file, status, originatorId,
                               originatorType, parent),
        HardwareIntf(bus, objPath.c_str(), HardwareIntf::action::defer_emit)
    {
        errorLogId(eid);
        failingUnitId(failingUnit);
        this->openpower::dump::hardware::HardwareIntf::emit_object_added();
    }
};
} // namespace hardware

namespace sbe
{

using SBEIntf =
    sdbusplus::server::object_t<sdbusplus::com::ibm::Dump::Entry::server::SBE>;

/** @class Entry
 *  @brief SBE Dump Entry implementation.
 *  @details A concrete implementation for the SBE dump type DBus API.
 *           This class extends the general dump entry with SBE-specific
 *           attributes, such as error log ID and failing unit ID.
 */
class Entry : public virtual openpower::dump::Entry, public virtual SBEIntf
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    ~Entry() = default;

    /** @brief Constructor for the SBE Dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to.
     *  @param[in] dumpId - Unique identifier for the dump.
     *  @param[in] timeStamp - Dump creation timestamp since the epoch.
     *  @param[in] fileSize - Size of the dump file in bytes.
     *  @param[in] file - Path to the dump file.
     *  @param[in] status - Current status of the dump.
     *  @param[in] originatorId - Identifier of the originator of the dump.
     *  @param[in] originatorType - Type of the originator.
     *  @param[in] eid - Error log identifier associated with the dump.
     *  @param[in] failingUnit - Identifier of the failing unit associated with
     *  the dump.
     *  @param[in] parent - Reference to the managing dump manager.
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t fileSize,
          const std::filesystem::path& file,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, uint64_t eid, uint64_t failingUnit,
          phosphor::dump::Manager& parent) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, fileSize,
                              file, status, originatorId, originatorType,
                              parent),
        openpower::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp,
                               fileSize, file, status, originatorId,
                               originatorType, parent),
        SBEIntf(bus, objPath.c_str(), SBEIntf::action::defer_emit)
    {
        errorLogId(eid);
        failingUnitId(failingUnit);
        this->openpower::dump::sbe::SBEIntf::emit_object_added();
    }
};
} // namespace sbe

} // namespace openpower::dump
