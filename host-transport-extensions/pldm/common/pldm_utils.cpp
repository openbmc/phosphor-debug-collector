// SPDX-License-Identifier: Apache-2.0

#include "pldm_utils.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <libpldm/transport.h>
#include <libpldm/transport/mctp-demux.h>
#include <poll.h>

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
pldm_transport* pldmTransport = nullptr;
pldm_transport_mctp_demux* mctpDemux = nullptr;

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
        elog<NotAllowed>(Reason(
            "Required host dump action via pldm is not allowed due "
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
        elog<NotAllowed>(Reason(
            "Failure in communicating with libpldm service, "
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

int openPLDM(mctp_eid_t eid)
{
    auto fd = -1;
    if (pldmTransport)
    {
        lg2::error("open: pldmTransport already setup!");
        elog<NotAllowed>(Reason(
            "Required host dump action via pldm is not allowed due "
            "to openPLDM failed"));
        return fd;
    }

    fd = openMctpDemuxTransport(eid);
    if (fd < 0)
    {
        auto e = errno;
        lg2::error("openPLDM failed, errno: {ERRNO}, FD: FD", "ERRNO", e, "FD",
                   fd);
        elog<NotAllowed>(Reason(
            "Required host dump action via pldm is not allowed due "
            "to openPLDM failed"));
    }
    return fd;
}

int openMctpDemuxTransport(mctp_eid_t eid)
{
    int rc = pldm_transport_mctp_demux_init(&mctpDemux);
    if (rc)
    {
        lg2::error(
            "openMctpDemuxTransport: Failed to init MCTP demux transport. rc = {RC}",
            "RC", rc);
        return rc;
    }

    rc = pldm_transport_mctp_demux_map_tid(mctpDemux, eid, eid);
    if (rc)
    {
        lg2::error(
            "openMctpDemuxTransport: Failed to setup tid to eid mapping. rc = {RC}",
            "RC", rc);
        pldmClose();
        return rc;
    }
    pldmTransport = pldm_transport_mctp_demux_core(mctpDemux);

    struct pollfd pollfd;
    rc = pldm_transport_mctp_demux_init_pollfd(pldmTransport, &pollfd);
    if (rc)
    {
        lg2::error("openMctpDemuxTransport: Failed to get pollfd. rc = {RC}",
                   "RC", rc);
        pldmClose();
        return rc;
    }
    return pollfd.fd;
}

void pldmClose()
{
    pldm_transport_mctp_demux_destroy(mctpDemux);
    mctpDemux = nullptr;
    pldmTransport = nullptr;
}

} // namespace pldm
} // namespace dump
} // namespace phosphor
