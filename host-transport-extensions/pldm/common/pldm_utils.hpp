// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "xyz/openbmc_project/Common/error.hpp"

#include <libpldm/instance-id.h>
#include <libpldm/pldm.h>

#include <phosphor-logging/lg2.hpp>

#include <system_error>

namespace phosphor
{
namespace dump
{
namespace pldm
{
/**
 * @brief Opens the PLDM file descriptor
 *
 * @return file descriptor on success and throw
 *         exception (xyz::openbmc_project::Common::Error::NotAllowed) on
 *         failures.
 */
int openPLDM();

/**
 * @brief RAII Class to get pldm instance ID
 *
 **/
class PLDMInstanceIdDb
{
  public:
    /** @brief Constructor - Initializes and allocates instance Id
     *  @param[in] tid - the terminus ID the instance ID is associated with
     *  @return - None
     */
    PLDMInstanceIdDb(pldm_tid_t endpointID = 1)
    {
        tid = endpointID;
        int rc = pldm_instance_db_init_default(&pldmInstanceIdDb);

        if (rc)
        {
            throw std::system_category().default_error_condition(rc);
        }
        allocateInstanceId();
    }

    /** @brief Destructor - Frees and destroys the instance Id
     *  @param[in] tid - the terminus ID the instance ID is associated with
     *  @return - None
     */
    ~PLDMInstanceIdDb()
    {
        freeInstanceId();
        int rc = pldm_instance_db_destroy(pldmInstanceIdDb);

        if (rc)
        {
            lg2::error("pldm_instance_db_destroy failed, RC: {RC}", "RC", rc);
        }
    }

    /** @brief getInstanceID - returns the instance ID allocated for this object
     *  @param[in] Unused
     *  @return - uint8_t InstanceID
     */
    uint8_t getInstanceID() const
    {
        return instanceID;
    }

  private:
    /** @brief Allocate an instance ID for the given terminus
     *  @param[in] tid - the terminus ID the instance ID is associated with
     *  @return - PLDM instance id or -EAGAIN if there are no available instance
     *            IDs
     */
    void allocateInstanceId()
    {
        int rc = pldm_instance_id_alloc(pldmInstanceIdDb, tid, &instanceID);

        if (rc == -EAGAIN)
        {
            throw std::runtime_error("No free instance ids");
        }

        if (rc)
        {
            throw std::system_category().default_error_condition(rc);
        }
    }

    /** @brief Mark an instance id as unused
     *  @param[in] Unused
     */
    void freeInstanceId()
    {
        int rc = pldm_instance_id_free(pldmInstanceIdDb, tid, instanceID);

        if (rc == -EINVAL)
        {
            throw std::runtime_error(
                "Instance ID " + std::to_string(instanceID) + " for TID " +
                std::to_string(tid) + " was not previously allocated");
        }
        if (rc)
        {
            throw std::system_category().default_error_condition(rc);
        }
    }

    pldm_instance_db* pldmInstanceIdDb = nullptr;
    // The terminus id for pldm is 1
    pldm_tid_t tid;
    pldm_instance_id_t instanceID;
};

} // namespace pldm
} // namespace dump
} // namespace phosphor
