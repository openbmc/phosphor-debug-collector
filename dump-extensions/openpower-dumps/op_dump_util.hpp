#pragma once

#include "dump_utils.hpp"
namespace openpower
{
namespace dump
{
namespace util
{

/** @brief Package an OpenPOWER dump files from a location
 *  into the dump format with dump header.
 *  @param[in] dumpId - Id of the dump
 *  @param[in] allowedSize - Size available for the dump
 *  @param[in] inputDir - Locations of input dump pieces
 *  @param[in] packageDir - Location where packaged dump should be stored
 *  @param[in] dumpPrefix - Dump filename prefix
 *  @param[in] event - sd_event_loop
 */
void captureDump(uint32_t dumpId, size_t allowedSize,
                 const std::string& inputDir, const std::string& packageDir,
                 const std::string& dumpPrefix,
                 const phosphor::dump::EventPtr& event);

/** @brief Get OpenPOWER dump policy
 *
 * @return true if dump is enabled and false if
 * dump is not enabled, if this service is not running
 * then default value is true
 */
bool getOPDumpPolicy();

} // namespace util
} // namespace dump
} // namespace openpower
