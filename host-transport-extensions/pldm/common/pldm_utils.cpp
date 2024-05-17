// SPDX-License-Identifier: Apache-2.0

#include "pldm_utils.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <libpldm/transport.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace dump
{
namespace pldm
{

using namespace phosphor::logging;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

pldm_instance_db* pldmInstanceIdDb = nullptr;

PLDMInstanceManager::PLDMInstanceManager()
{
    initPLDMInstanceIdDb();
}

PLDMInstanceManager::~PLDMInstanceManager()
{
    destroyPLDMInstanceIdDb();
}

void PLDMInstanceManager::initPLDMInstanceIdDb()
{
    auto rc = pldm_instance_db_init_default(&pldmInstanceIdDb);
    if (rc)
    {
        lg2::error("Error calling pldm_instance_db_init_default, rc = {RC}",
                   "RC", rc);
        elog<NotAllowed>(
            Reason("Required host dump action via pldm is not allowed due "
                   "to pldm_open failed"));
    }
}

void PLDMInstanceManager::destroyPLDMInstanceIdDb()
{
    auto rc = pldm_instance_db_destroy(pldmInstanceIdDb);
    if (rc)
    {
        lg2::error("pldm_instance_db_destroy failed rc = {RC}", "RC", rc);
    }
}

pldm_instance_id_t getPLDMInstanceID(uint8_t tid)
{
    pldm_instance_id_t instanceID = 0;

    auto rc = pldm_instance_id_alloc(pldmInstanceIdDb, tid, &instanceID);
    if (rc == -EAGAIN)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        rc = pldm_instance_id_alloc(pldmInstanceIdDb, tid, &instanceID);
    }

    if (rc)
    {
        lg2::error("Failed to get instance id, rc = {RC}", "RC", rc);
        elog<NotAllowed>(
            Reason("Failure in communicating with libpldm service, "
                   "service may not be running"));
    }
    lg2::info("Got instanceId: {INSTANCE_ID} from PLDM eid: {EID}",
              "INSTANCE_ID", instanceID, "EID", tid);

    return instanceID;
}

void freePLDMInstanceID(pldm_instance_id_t instanceID, uint8_t tid)
{
    auto rc = pldm_instance_id_free(pldmInstanceIdDb, tid, instanceID);
    if (rc)
    {
        lg2::error(
            "pldm_instance_id_free failed to free id = {ID} of tid = {TID} rc = {RC}",
            "ID", instanceID, "TID", tid, "RC", rc);
    }
}

int openPLDM()
{
    auto fd = pldm_open();
    if (fd < 0)
    {
        auto e = errno;
        lg2::error("pldm_open failed, errno: {ERRNO}, FD: FD", "ERRNO", e, "FD",
                   fd);
        elog<NotAllowed>(Reason(
            "Required host dump action via pldm is not allowed due "
            "to pldm_open failed"));
    }
    return fd;
}

} // namespace pldm
} // namespace dump
} // namespace phosphor
