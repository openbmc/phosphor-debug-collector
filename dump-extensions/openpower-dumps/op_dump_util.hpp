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
 * A xyz.openbmc_project.Dump.Create.Error.Disabled will be thrown
 * if the dumps are disabled.
 * If the settings service is not running then considering as
 * the dumps are enabled.
 */
void isOPDumpsEnabled();

} // namespace util
} // namespace dump
} // namespace openpower
