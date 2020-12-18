/**
 * Copyright © 2019 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pldm_oem_cmds.hpp"

#include "dump_utils.hpp"
#include "pldm_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <libpldm/base.h>
#include <libpldm/platform.h>
#include <unistd.h>

#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

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
void requestOffload(uint32_t id)
{
    pldm::requestOffload(id);
}

void requestDelete(uint32_t id, uint32_t dumpType)
{
    pldm::requestDelete(id, dumpType);
}
} // namespace host

namespace pldm
{

using namespace phosphor::logging;

constexpr auto eidPath = "/usr/share/pldm/host_eid";
constexpr mctp_eid_t defaultEIDValue = 9;

using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

mctp_eid_t readEID()
{
    mctp_eid_t eid = defaultEIDValue;

    std::ifstream eidFile{eidPath};
    if (!eidFile.good())
    {
        log<level::ERR>("Could not open host EID file");
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
            log<level::ERR>("EID file was empty");
            elog<NotAllowed>(
                Reason("Required host dump action via pldm is not "
                       "allowed due to mctp end point read failed"));
        }
    }

    return eid;
}

void requestOffload(uint32_t id)
{
    uint16_t effecterId = 0x05; // TODO PhyP temporary Hardcoded value.

    std::array<uint8_t, sizeof(pldm_msg_hdr) + sizeof(effecterId) + sizeof(id) +
                            sizeof(uint8_t)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    std::array<uint8_t, sizeof(id)> effecterValue{};

    memcpy(effecterValue.data(), &id, sizeof(id));

    mctp_eid_t eid = readEID();

    auto instanceID = getPLDMInstanceID(eid);

    auto rc = encode_set_numeric_effecter_value_req(
        instanceID, effecterId, PLDM_EFFECTER_DATA_SIZE_UINT32,
        effecterValue.data(), request,
        requestMsg.size() - sizeof(pldm_msg_hdr));

    if (rc != PLDM_SUCCESS)
    {
        log<level::ERR>("Message encode failure. ", entry("RC=%d", rc));
        elog<NotAllowed>(Reason("Host dump offload via pldm is not "
                                "allowed due to encode failed"));
    }

    uint8_t* responseMsg = nullptr;
    size_t responseMsgSize{};

    CustomFd fd(openPLDM());

    rc = pldm_send_recv(eid, fd(), requestMsg.data(), requestMsg.size(),
                        &responseMsg, &responseMsgSize);
    if (rc < 0)
    {
        auto e = errno;
        log<level::ERR>("pldm_send failed", entry("RC=%d", rc),
                        entry("ERRNO=%d", e));
        elog<NotAllowed>(Reason("Host dump offload via pldm is not "
                                "allowed due to fileack send failed"));
    }
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg);
    log<level::INFO>(
        "Done. PLDM message",
        entry("RC=%d", static_cast<uint16_t>(response->payload[0])));
}

void requestDelete(uint32_t dumpId, uint32_t dumpType)
{
    pldm_fileio_file_type pldmDumpType;
    switch (dumpType)
    {
        case PLDM_FILE_TYPE_DUMP:
            pldmDumpType = PLDM_FILE_TYPE_DUMP;
            break;
        case PLDM_FILE_TYPE_RESOURCE_DUMP_PARMS:
            pldmDumpType = PLDM_FILE_TYPE_RESOURCE_DUMP_PARMS;
            break;
        default:
            throw std::runtime_error("Unknown pldm dump file-io type to delete "
                                     "host dump");
    }
    const size_t pldmMsgHdrSize = sizeof(pldm_msg_hdr);
    std::array<uint8_t, pldmMsgHdrSize + PLDM_FILE_ACK_REQ_BYTES> fileAckReqMsg;

    mctp_eid_t mctpEndPointId = readEID();

    auto pldmInstanceId = getPLDMInstanceID(mctpEndPointId);

    // - PLDM_SUCCESS - To indicate dump was readed (offloaded) or user decided,
    //   no longer host dump is not required so, initiate deletion from
    //   host memory
    int retCode =
        encode_file_ack_req(pldmInstanceId, pldmDumpType, dumpId, PLDM_SUCCESS,
                            reinterpret_cast<pldm_msg*>(fileAckReqMsg.data()));

    if (retCode != PLDM_SUCCESS)
    {
        log<level::ERR>("Failed to encode pldm FileAck to delete host dump",
                        entry("SRC_DUMP_ID=%d", dumpId),
                        entry("PLDM_FILE_IO_TYPE=%d", pldmDumpType),
                        entry("PLDM_RETURN_CODE=%d", retCode));
        elog<NotAllowed>(Reason("Host dump deletion via pldm is not "
                                "allowed due to encode fileack failed"));
    }

    uint8_t* pldmRespMsg = nullptr;
    size_t pldmRespMsgSize;

    CustomFd pldmFd(openPLDM());

    retCode =
        pldm_send_recv(mctpEndPointId, pldmFd(), fileAckReqMsg.data(),
                       fileAckReqMsg.size(), &pldmRespMsg, &pldmRespMsgSize);

    std::unique_ptr<uint8_t, decltype(std::free)*> pldmRespMsgPtr{pldmRespMsg,
                                                                  std::free};
    if (retCode != PLDM_REQUESTER_SUCCESS)
    {
        auto errorNumber = errno;
        log<level::ERR>("Failed to send pldm FileAck to delete host dump",
                        entry("SRC_DUMP_ID=%d", dumpId),
                        entry("PLDM_FILE_IO_TYPE=%d", pldmDumpType),
                        entry("PLDM_RETURN_CODE=%d", retCode),
                        entry("ERRNO=%d", errorNumber),
                        entry("ERRMSG=%s", strerror(errorNumber)));
        elog<NotAllowed>(Reason("Host dump deletion via pldm is not "
                                "allowed due to fileack send failed"));
    }

    uint8_t completionCode;

    retCode =
        decode_file_ack_resp(reinterpret_cast<pldm_msg*>(pldmRespMsgPtr.get()),
                             pldmRespMsgSize - pldmMsgHdrSize, &completionCode);

    if (retCode || completionCode)
    {
        log<level::ERR>("Failed to delete host dump",
                        entry("SRC_DUMP_ID=%d", dumpId),
                        entry("PLDM_FILE_IO_TYPE=%d", pldmDumpType),
                        entry("PLDM_RETURN_CODE=%d", retCode),
                        entry("PLDM_COMPLETION_CODE=%d", completionCode));
        elog<NotAllowed>(Reason("Host dump deletion via pldm is "
                                "failed"));
    }

    log<level::INFO>("Deleted host dump", entry("SRC_DUMP_ID=%d", dumpId));
}
} // namespace pldm
} // namespace dump
} // namespace phosphor
