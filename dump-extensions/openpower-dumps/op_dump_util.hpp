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

} // namespace util
} // namespace dump
} // namespace openpower
