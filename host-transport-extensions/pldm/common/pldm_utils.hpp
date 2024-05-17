// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <libpldm/instance-id.h>
#include <libpldm/pldm.h>

namespace phosphor
{
namespace dump
{
namespace pldm
{

class PLDMInstanceManager
{
  public:
    PLDMInstanceManager(const PLDMInstanceManager&) = delete;
    PLDMInstanceManager& operator=(const PLDMInstanceManager&) = delete;

    PLDMInstanceManager();
    ~PLDMInstanceManager();

  private:
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
};

/**
 * @brief Opens the PLDM file descriptor
 *
 * @return file descriptor on success and throw
 *         exception (xyz::openbmc_project::Common::Error::NotAllowed) on
 *         failures.
 */
int openPLDM();

/**
 * @brief Returns the PLDM instance ID to use for PLDM commands
 *
 * @param[in] tid - the terminus ID the instance ID is associated with
 *
 * @return pldm_instance_id_t - The instance ID
 **/
pldm_instance_id_t getPLDMInstanceID(uint8_t tid);

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
