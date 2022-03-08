#pragma once

#include "dump_utils.hpp"

namespace openpower
{
namespace dump
{
namespace util
{

/** @brief Check whether OpenPOWER dumps are enabled
 *
 * param[in] bus - D-Bus handle
 *
 * If the settings service is not running then considering as
 * the dumps are enabled.
 * @return true - if dumps are enabled, false - if dumps are not enabled
 */
bool isOPDumpsEnabled(sdbusplus::bus::bus& bus);

using BIOSAttrValueType = std::variant<int64_t, std::string>;

/** @brief Read a BIOS attribute value
 *
 *  @param[in] attrName - Name of the BIOS attribute
 *  @param[in] bus - D-Bus handle
 *
 *  @return The value of the BIOS attribute as a variant of possible types
 *
 *  @throws sdbusplus::exception::SdBusError if failed to read the attribute
 */
BIOSAttrValueType readBIOSAttribute(const std::string& attrName,
                                    sdbusplus::bus::bus& bus);

/** @brief Check whether memory preserving reboot is in progress
 *  @return true - memory preserving reboot in progress
 *          false - no memory preserving reboot is in progress
 */
bool isInMpReboot();

/** @brief Check whether a system is in progress or available to offload.
 *
 *  @param[in] bus - D-Bus handle
 *
 *  @return true - A dump is in progress or available to offload
 *          false - No dump in progress
 */
bool isSystemDumpInProgress(sdbusplus::bus::bus& bus);
} // namespace util
} // namespace dump
} // namespace openpower
