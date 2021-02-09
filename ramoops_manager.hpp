#pragma once

#include "config.h"

#include <phosphor-logging/log.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace phosphor::logging;

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
     *         D-bus internal create interface.
     *  @param [in] files - Core files list
     */
    void createHelper(const std::vector<std::string>& files);
};

} // namespace ramoops
} // namespace dump
} // namespace phosphor
