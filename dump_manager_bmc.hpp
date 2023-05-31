#pragma once

#include "dump_manager.hpp"
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

using CreateIface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Dump::server::Create>;

using UserMap = phosphor::dump::inotify::UserMap;

using Watch = phosphor::dump::inotify::Watch;
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
        eventLoop(event.get()),
        dumpWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            filePath,
            std::bind(std::mem_fn(&phosphor::dump::bmc::Manager::watchCallback),
                      this, std::placeholders::_1)),
        dumpDir(filePath)
    {}

    /** @brief Implementation of dump watch call back
     *  @param [in] fileInfo - map of file info  path:event
     */
    void watchCallback(const UserMap& fileInfo);

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    void restore() override;

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

  private:
    /** @brief Create Dump entry d-bus object
     *  @param[in] fullPath - Full path of the Dump file name
     */
    void createEntry(const std::filesystem::path& fullPath);

    /**  @brief Capture BMC Dump based on the Dump type.
     *  @param[in] type - Type of the Dump.
     *  @param[in] path - An absolute paths to the file
     *             to be included as part of Dump package.
     *  @return id - The Dump entry id number.
     */
    uint32_t captureDump(std::string& type, const std::string& path);

    /** @brief Remove specified watch object pointer from the
     *        watch map and associated entry from the map.
     *        @param[in] path - unique identifier of the map
     */
    void removeWatch(const std::filesystem::path& path);

    /** @brief Calculate per dump allowed size based on the available
     *        size in the dump location.
     *  @returns dump size in kilobytes.
     */
    size_t getAllowedSize();

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
                log<level::ERR>(
                    fmt::format("An invalid input  passed for ({})", key)
                        .c_str());
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
        const auto mapIt = dumpTypeMap.find(type);
        if (mapIt != dumpTypeMap.end())
        {
            dumpType = mapIt->second;
        }
        else
        {
            log<level::ERR>(
                fmt::format("An invalid dump type passed ({})", type).c_str());
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

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Dump main watch object */
    Watch dumpWatch;

    /** @brief Path to the dump file*/
    std::string dumpDir;

    /** @brief Flag to reject user intiated dump if a dump is in progress*/
    // TODO: https://github.com/openbmc/phosphor-debug-collector/issues/19
    static bool fUserDumpInProgress;

    /** @brief Child directory path and its associated watch object map
     *        [path:watch object]
     */
    std::map<std::filesystem::path, std::unique_ptr<Watch>> childWatchMap;

    /** @brief map of SDEventPlus child pointer added to event loop */
    std::map<pid_t, std::unique_ptr<Child>> childPtrMap;
};

} // namespace bmc
} // namespace dump
} // namespace phosphor
