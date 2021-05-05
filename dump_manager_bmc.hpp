#pragma once

#include "dump_manager_bmcstored.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"

#include <xyz/openbmc_project/Dump/Create/server.hpp>

#include <filesystem>

namespace phosphor
{
namespace dump
{
namespace bmc
{
namespace internal
{

class Manager;

} // namespace internal

constexpr auto BMC_DUMP_FILENAME_REGEX =
    "obmcdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)";
using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

using Type =
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;

// Type to dreport type  string map
static const std::map<Type, std::string> TypeMap = {
    {Type::ApplicationCored, "core"},
    {Type::UserRequested, "user"},
    {Type::InternalFailure, "elog"},
    {Type::Checkstop, "checkstop"},
    {Type::Ramoops, "ramoops"}};

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API
 */
class Manager :
    virtual public CreateIface,
    virtual public phosphor::dump::bmc_stored::Manager
{
    friend class internal::Manager;

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
     *  @param[in] baseEntryPath - Base path for dump entry.
     *  @param[in] filePath - Path where the dumps are stored.
     */
    Manager(sdbusplus::bus::bus& bus, const EventPtr& event, const char* path,
            const std::string& baseEntryPath, const char* filePath) :
        CreateIface(bus, path),
        phosphor::dump::bmc_stored::Manager(
            bus, event, path, baseEntryPath, filePath, BMC_DUMP_FILENAME_REGEX,
            BMC_DUMP_MAX_SIZE, BMC_DUMP_MIN_SPACE_REQD, BMC_DUMP_TOTAL_SIZE)
    {}

    /** @brief Implementation for CreateDump
     *  Method to create a BMC dump entry when user requests for a new BMC dump
     *
     *  @return object_path - The object path of the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

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
    void createEntry(const uint32_t id, const std::string objPath,
                     const uint64_t ms, uint64_t fileSize,
                     const std::filesystem::path& file,
                     phosphor::dump::OperationStatus status) override;

  private:
    /**  @brief Capture BMC Dump based on the Dump type.
     *  @param[in] type - Type of the Dump.
     *  @param[in] fullPaths - List of absolute paths to the files
     *             to be included as part of Dump package.
     *  @return id - The Dump entry id number.
     */
    uint32_t captureDump(Type type, const std::vector<std::string>& fullPaths);
};

} // namespace bmc
} // namespace dump
} // namespace phosphor
