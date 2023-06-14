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
#include <libpldm/file_io.h>
#include <libpldm/platform.h>
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
        lg2::error("Message encode failure. RC: {RC}", "RC", rc);
        elog<NotAllowed>(Reason("Host dump offload via pldm is not "
                                "allowed due to encode failed"));
    }

    CustomFd fd(openPLDM());

    lg2::info("Sending request to offload dump id: {ID}, eid: {EID}", "ID", id,
              "EID", eid);

    rc = pldm_send(eid, fd(), requestMsg.data(), requestMsg.size());
    if (rc < 0)
    {
        auto e = errno;
        lg2::error("pldm_send failed, RC: {RC}, errno: {ERRNO}", "RC", rc,
                   "ERRNO", e);
        elog<NotAllowed>(Reason("Host dump offload via pldm is not "
                                "allowed due to fileack send failed"));
    }
    lg2::info("Done. PLDM message, id: {ID}, RC: {RC}", "ID", id, "RC", rc);
}

void requestDelete(uint32_t dumpId, uint32_t dumpType)
{
    pldm_fileio_file_type pldmDumpType;
    switch (dumpType)
    {
        case PLDM_FILE_TYPE_DUMP:
            pldmDumpType = PLDM_FILE_TYPE_DUMP;
            break;
        case PLDM_FILE_TYPE_RESOURCE_DUMP:
            pldmDumpType = PLDM_FILE_TYPE_RESOURCE_DUMP;
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
        lg2::error(
            "Failed to encode pldm FileAck to delete host dump, "
            "SRC_DUMP_ID: {SRC_DUMP_ID}, PLDM_FILE_IO_TYPE: {PLDM_DUMP_TYPE}, "
            "PLDM_RETURN_CODE: {RET_CODE}",
            "SRC_DUMP_ID", dumpId, "PLDM_DUMP_TYPE",
            static_cast<std::underlying_type<pldm_fileio_file_type>::type>(
                pldmDumpType),
            "RET_CODE", retCode);
        elog<NotAllowed>(Reason("Host dump deletion via pldm is not "
                                "allowed due to encode fileack failed"));
    }

    CustomFd pldmFd(openPLDM());

    retCode = pldm_send(mctpEndPointId, pldmFd(), fileAckReqMsg.data(),
                        fileAckReqMsg.size());
    if (retCode != PLDM_REQUESTER_SUCCESS)
    {
        auto errorNumber = errno;
        lg2::error(
            "Failed to send pldm FileAck to delete host dump, "
            "SRC_DUMP_ID: {SRC_DUMP_ID}, PLDM_FILE_IO_TYPE: {PLDM_DUMP_TYPE}, "
            "PLDM_RETURN_CODE: {RET_CODE}, ERRNO: {ERRNO}, ERRMSG: {ERRMSG}",
            "SRC_DUMP_ID", dumpId, "PLDM_DUMP_TYPE",
            static_cast<std::underlying_type<pldm_fileio_file_type>::type>(
                pldmDumpType),
            "RET_CODE", retCode, "ERRNO", errorNumber, "ERRMSG",
            strerror(errorNumber));
        elog<NotAllowed>(Reason("Host dump deletion via pldm is not "
                                "allowed due to fileack send failed"));
    }

    lg2::info(
        "Sent request to host to delete the dump, SRC_DUMP_ID: {SRC_DUMP_ID}",
        "SRC_DUMP_ID", dumpId);
}
} // namespace pldm
} // namespace dump
} // namespace phosphor
