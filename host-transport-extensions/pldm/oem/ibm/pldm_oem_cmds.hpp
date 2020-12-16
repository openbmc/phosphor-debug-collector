#pragma once

#include <libpldm/pldm.h>

namespace phosphor
{
namespace dump
{
namespace host
{
/**
 * @brief Initiate offload of the dump with provided id
 *
 * @param[in] id - The Dump Source ID.
 *
 */
void requestOffload(uint32_t id);
} // namespace host

namespace pldm
{

/**
 * PLDMInterface
 *
 * Handles sending the SetNumericEffecterValue PLDM
 * command to the host to start dump offload.
 *
 */

/**
 * @brief Kicks of the SetNumericEffecterValue command to
 *        start offload the dump
 *
 * @param[in] id - The Dump Source ID.
 *
 */

void requestOffload(uint32_t id);

/**
 * @brief Reads the MCTP endpoint ID out of a file
 */
mctp_eid_t readEID();

} // namespace pldm
} // namespace dump
} // namespace phosphor
