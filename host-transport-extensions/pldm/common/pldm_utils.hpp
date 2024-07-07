// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <libpldm/instance-id.h>
#include <libpldm/pldm.h>
#include <libpldm/transport.h>

namespace phosphor
{
namespace dump
{
namespace pldm
{

constexpr auto eidPath = "/usr/share/pldm/host_eid";
constexpr mctp_eid_t defaultEIDValue = 9;
using TerminusID = uint8_t;
constexpr TerminusID tid = defaultEIDValue;

extern struct pldm_transport* pldmTransport;

class PLDMInstanceManager
{
  public:
    PLDMInstanceManager(const PLDMInstanceManager&) = delete;
    PLDMInstanceManager& operator=(const PLDMInstanceManager&) = delete;

    PLDMInstanceManager();
    ~PLDMInstanceManager();
};

/**
 * @brief setup PLDM transport for sending and receiving messages
 *
 * @return file descriptor on success and throw
 *         exception (xyz::openbmc_project::Common::Error::NotAllowed) on
 *         failures.
 */
int openPLDM();

/** @brief Opens the MCTP socket for sending and receiving messages.
 *
 */
int openMctpDemuxTransport();

/** @brief Opens the MCTP AF_MCTP for sending and receiving messages.
 *
 */
int openAfMctpTransport();

/** @brief Close the PLDM file */
void pldmClose();

/**
 * @brief Instantiates an instance ID database object
 *
 * @return void
 **/
void initPLDMInstanceIdDb();

/**
 * @brief Destroys an instance ID database object
 *
 * @return void
 **/
void destroyPLDMInstanceIdDb();

/**
 * @brief Returns the PLDM instance ID to use for PLDM commands
 *
 * @param[in] tid - the terminus ID the instance ID is associated with
 *
 * @return pldm_instance_id_t - The instance ID
 **/
uint8_t getPLDMInstanceID(uint8_t tid);

/**
 * @brief Free the PLDM instance ID
 *
 * @param[in] tid - the terminus ID the instance ID is associated with
 * @param[in] instanceID - The PLDM instance ID
 *
 * @return void
 **/
void freePLDMInstanceID(pldm_instance_id_t instanceID, uint8_t tid);

} // namespace pldm
} // namespace dump
} // namespace phosphor
