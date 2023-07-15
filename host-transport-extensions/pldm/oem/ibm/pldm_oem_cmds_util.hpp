#pragma once

#include "dump_utils.hpp"
#include "pldm_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <libpldm/base.h>
#include <libpldm/file_io.h>
#include <libpldm/platform.h>
#include <libpldm/pldm.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <fstream>

namespace phosphor
{
namespace dump
{
namespace host
{

using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;
constexpr auto eidPath = "/usr/share/pldm/host_eid";
constexpr mctp_eid_t defaultEIDValue = 9;
constexpr uint16_t EFFECTER_ID = 0x05;
constexpr size_t PLDM_MSG_HDR_SIZE = sizeof(pldm_msg_hdr);

/**
 * @brief Reads the MCTP endpoint ID out of a file
 */
inline mctp_eid_t readEID()
{
    mctp_eid_t eid = defaultEIDValue;

    std::ifstream eidFile{eidPath};
    if (!eidFile.good())
    {
        lg2::error("Could not open host EID file");
        elog<NotAllowed>(Reason("Required host dump action via pldm is not "
                                "allowed due to mctp end point read failed"));
    }
    else
    {
        std::string eid;
        eidFile >> eid;
        if (!eid.empty())
        {
            eid = strtol(eid.c_str(), nullptr, 10);
        }
        else
        {
            lg2::error("EID file was empty");
            elog<NotAllowed>(
                Reason("Required host dump action via pldm is not "
                       "allowed due to mctp end point read failed"));
        }
    }

    return eid;
}

inline void handleEncodeFailure(int rc)
{
    if (rc != PLDM_SUCCESS)
    {
        lg2::error("Message encode failure. RC: {RC}", "RC", rc);
        elog<NotAllowed>(Reason("Host dump offload via pldm is not "
                                "allowed due to encode failed"));
    }
}

inline void handlePLDMSendFailure(int rc)
{
    if (rc < 0)
    {
        auto e = errno;
        lg2::error("pldm_send failed, RC: {RC}, errno: {ERRNO}", "RC", rc,
                   "ERRNO", e);
        elog<NotAllowed>(Reason("Host dump offload via pldm is not "
                                "allowed due to fileack send failed"));
    }
}

inline void createRequestMsg(pldm_msg* request, uint32_t id)
{
    std::array<uint8_t, PLDM_MSG_HDR_SIZE + sizeof(EFFECTER_ID) + sizeof(id) +
                            sizeof(uint8_t)>
        requestMsg{};
    request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    std::array<uint8_t, sizeof(id)> effecterValue{};
    memcpy(effecterValue.data(), &id, sizeof(id));

    mctp_eid_t eid = readEID();
    auto instanceID = phosphor::dump::pldm::getPLDMInstanceID(eid);

    auto rc = encode_set_numeric_effecter_value_req(
        instanceID, EFFECTER_ID, PLDM_EFFECTER_DATA_SIZE_UINT32,
        effecterValue.data(), request, requestMsg.size() - PLDM_MSG_HDR_SIZE);

    handleEncodeFailure(rc);
}

inline void createFileAckReqMsg(pldm_msg* request,
                                pldm_fileio_file_type pldmDumpType,
                                uint32_t dumpId, mctp_eid_t mctpEndPointId)
{
    auto pldmInstanceId =
        phosphor::dump::pldm::getPLDMInstanceID(mctpEndPointId);

    auto retCode = encode_file_ack_req(pldmInstanceId, pldmDumpType, dumpId,
                                       PLDM_SUCCESS, request);

    handleEncodeFailure(retCode);
}
} // namespace host
} // namespace dump
} // namespace phosphor
