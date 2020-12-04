// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <libpldm/pldm.h>

namespace phosphor
{
namespace dump
{
namespace pldm
{

/**
 * @brief Opens the PLDM file descriptor
 */
int openPLDM();

/**
 * @brief Returns the PLDM instance ID to use for PLDM commands
 *
 * @param[in] eid - The PLDM EID
 *
 * @return uint8_t - The instance ID
 **/
uint8_t getPLDMInstanceID(uint8_t eid);

} // namespace pldm
} // namespace dump
} // namespace phosphor
