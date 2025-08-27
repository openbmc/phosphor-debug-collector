#pragma once

#include "config.h"

#include <filesystem>
#include <string>
#include <vector>

namespace phosphor
{
namespace dump
{
namespace ramoops
{

/** @class Manager
 *  @brief OpenBMC Core manager implementation.
 */
class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = default;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to create ramoops
     *  @param[in] filePath - Path where the ramoops are stored.
     */
    Manager(const std::string& filePath);

  private:
    /** @brief Helper function for initiating dump request using
     *         createDump D-Bus interface.
     *  @param [in] files - ramoops files list
     */
    void createHelper(const std::vector<std::string>& files);

    /** @brief Create an error indicating ramoops was found
     *
     */
    void createError();

    /** @brief Rename file to avoid duplicate records
     *  @param [in] dir - ramoops dir
     */
    void prefixFilesWithLast(const std::filesystem::path& dir);
};

} // namespace ramoops
} // namespace dump
} // namespace phosphor
