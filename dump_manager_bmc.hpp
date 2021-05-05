#pragma once

#include "dump_helper.hpp"
#include "dump_utils.hpp"
#include "errors_map.hpp"
#include "watch.hpp"

#include <sdeventplus/source/child.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

#include <filesystem>
#include <map>

namespace phosphor
{
namespace dump
{
namespace bmc
{

constexpr auto BMC_DUMP_FILENAME_REGEX =
    "obmcdump_([0-9]+)_([0-9]+).([a-zA-Z0-9]+)";

using CreateIface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

class DumpHelper;

using ::sdeventplus::source::Child;

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API
 */
class Manager :
    virtual public CreateIface,
    virtual public phosphor::dump::Manager
{
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
    Manager(sdbusplus::bus_t& bus, const EventPtr& event, const char* path,
            const std::string& baseEntryPath, const char* filePath) :
        CreateIface(bus, path),
        phosphor::dump::Manager(bus, path, baseEntryPath),
        dumpHelper(bus, event, filePath, BMC_DUMP_FILENAME_REGEX,
                   BMC_DUMP_MAX_SIZE, BMC_DUMP_MIN_SPACE_REQD,
                   BMC_DUMP_TOTAL_SIZE, this)
    {}

    // Declare DumpHelper as a friend class
    friend class DumpHelper;

    /** @brief Implementation for CreateDump
     *  Method to create a BMC dump entry when user requests for a new BMC dump
     *
     *  @return object_path - The object path of the new dump entry.
     */
    sdbusplus::message::object_path
        createDump(phosphor::dump::DumpCreateParams params) override;

    // Type to dreport type  string map
    static inline const std::unordered_map<std::string, std::string>
        dumpTypeMap = {
            {"xyz.openbmc_project.Dump.Create.DumpType.ErrorLog", "elog"},
            {"xyz.openbmc_project.Dump.Create.DumpType.ApplicationCored",
             "core"},
            {"xyz.openbmc_project.Dump.Create.DumpType.UserRequested", "user"},
            {"xyz.openbmc_project.Dump.Create.DumpType.Ramoops", "ramoops"}};

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
                     phosphor::dump::OperationStatus status,
                     std::string originatorId,
                     originatorTypes originatorType) override;

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    void restore() override
    {
        dumpHelper.restore();
    }

  private:
    /**  @brief Capture BMC Dump based on the Dump type.
     *  @param[in] type - Type of the Dump.
     *  @param[in] path - An absolute paths to the file
     *             to be included as part of Dump package.
     *  @return id - The Dump entry id number.
     */
    uint32_t captureDump(std::string& type, const std::string& path);

    /** @brief extract the dump create parameters
     *  @param[in] The name of the parameter
     *  @param[in] The map of parameters passed as input
     *
     *  @return The parameter value
     */
    template <typename T>
    T extractParameter(const std::string& key,
                       phosphor::dump::DumpCreateParams& params)
    {
        using InvalidArgument =
            sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
        using Argument = xyz::openbmc_project::Common::InvalidArgument;

        auto it = params.find(key);
        if (it != params.end())
        {
            const auto& [foundKey, variantValue] = *it;
            if (std::holds_alternative<T>(variantValue))
            {
                return std::get<T>(variantValue);
            }
            else
            {
                lg2::error("An invalid input  passed for key: {KEY}", "KEY",
                           key);
                elog<InvalidArgument>(
                    Argument::ARGUMENT_NAME(key.c_str()),
                    Argument::ARGUMENT_VALUE("INVALID INPUT"));
            }
        }
        return T{};
    }

    /** @brief This function validates the dump type and return correct type for
     * elog dump
     * @param[in] type The dump type recieved
     * @param[in] params The map of parameters passed as input
     *
     * @return A correct dump type will be returned
     */
    std::string validateDumpType(const std::string& type,
                                 phosphor::dump::DumpCreateParams& params)
    {
        using InvalidArgument =
            sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
        using Argument = xyz::openbmc_project::Common::InvalidArgument;
        std::string dumpType = "user";
        if (type.empty())
        {
            return dumpType;
        }
        const auto mapIt = dumpTypeMap.find(type);
        if (mapIt != dumpTypeMap.end())
        {
            dumpType = mapIt->second;
        }
        else
        {
            lg2::error("An invalid dump type: {TYPE} passed", "TYPE", type);
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("BMC_DUMP_TYPE"),
                                  Argument::ARGUMENT_VALUE(type.c_str()));
        }

        if (dumpType == "elog")
        {
            std::string errorType = extractParameter<std::string>(
                convertCreateParametersToString(CreateParameters::ErrorType),
                params);
            const auto elogIt = errorMap.find(errorType);
            if (elogIt != errorMap.end())
            {
                dumpType = errorType;
            }
        }
        return dumpType;
    }

    /** @brief Flag to reject user intiated dump if a dump is in progress*/
    // TODO: https://github.com/openbmc/phosphor-debug-collector/issues/19
    static bool fUserDumpInProgress;

    /** @brief map of SDEventPlus child pointer added to event loop */
    std::map<pid_t, std::unique_ptr<Child>> childPtrMap;

    phosphor::dump::util::DumpHelper dumpHelper;
};

} // namespace bmc
} // namespace dump
} // namespace phosphor
