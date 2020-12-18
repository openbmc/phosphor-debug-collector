#pragma once

#include <libpldm/file_io.h>
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

/**
 * @brief Request to delete dump
 *
 * @param[in] id - The Dump Source ID.
 * @param[in] dumpType - Type of the dump.
 * @return NULL
 *
 */
void requestDelete(uint32_t id, uint32_t dumpType);
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

/**
 * @brief Request to delete dump
 *
 * @param[in] id - The Dump Source ID.
 * @param[in] dumpType - Type of the dump.
 * @return NULL
 *
 */
void requestDelete(uint32_t id, pldm_fileio_file_type dumpType);
} // namespace pldm
} // namespace dump
} // namespace phosphor
