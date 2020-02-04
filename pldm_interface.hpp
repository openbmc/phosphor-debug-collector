#pragma once

//#include "host_interface.hpp"

#include <libpldm/pldm.h>

#include <memory>
#include <sdeventplus/source/io.hpp>

namespace phosphor
{
namespace dump
{
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

    void sendGetSysDumpCmd(uint32_t id);

    /**
     * @brief Reads the MCTP endpoint ID out of a file
     */
    mctp_eid_t readEID();

    /**
     * @brief Opens the PLDM file descriptor
     */
    int open();

    /**
     * @brief Closes the PLDM file descriptor
     */
    void closeFD( int fd);
} // namespace pldm
} // namespace dump
} // namespace phosphor
