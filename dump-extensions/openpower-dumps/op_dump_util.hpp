#pragma once

#include "dump_utils.hpp"
#include "types.hpp"

namespace openpower
{
namespace dump
{
namespace util
{

constexpr auto SBE_DUMP_TYPE_HOSTBOOT = 0x5;
constexpr auto SBE_DUMP_TYPE_HARDWARE = 0x1;
constexpr auto SBE_DUMP_TYPE_SBE = 0xA;

/** @brief Check whether OpenPOWER dumps are enabled
 *
 * param[in] bus - D-Bus handle
 *
 * If the settings service is not running then considering as
 * the dumps are enabled.
 * @return true - if dumps are enabled, false - if dumps are not enabled
 */
bool isOPDumpsEnabled(sdbusplus::bus::bus& bus);

/** @brief Check whether memory preserving reboot is in progress
 *  @return true - memory preserving reboot in progress
 *          false - no memory preserving reboot is in progress
 */
bool isInMpReboot();

/** @brief Check whether a system is in progress or
 *  available to offload.
 *  @return true - A dump is in progress or available to offload
 *          false - No dump in progress
 */
bool isSystemDumpInProgress();

/** @brief Extract passed dump create parameters
 *  @param[in] params - A map contain input parameters
 *  @param[in] dumpType - Type of the dump
 *  @param[out] eid - Error id associated with dump
 *  @param[out] failingUnit - Harware unit failed
 */
void extractDumpCreateParams(const phosphor::dump::DumpCreateParams& params,
                             uint8_t dumpType, uint64_t& eid,
                             uint64_t& failingUnit);
} // namespace util
} // namespace dump
} // namespace openpower
