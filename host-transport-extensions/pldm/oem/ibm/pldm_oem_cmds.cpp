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
#include "dump_utils.hpp"
#include "host_transport_exts.hpp"
#include "pldm_oem_cmds_util.hpp"
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

using namespace phosphor::logging;

void HostTransport::requestOffload(uint32_t id)
{
    pldm_msg* request;
    createRequestMsg(request, id);

    CustomFd fd(phosphor::dump::pldm::openPLDM());
    mctp_eid_t eid = readEID();

    lg2::info("Sending request to offload dump id: {ID}, eid: {EID}", "ID", id,
              "EID", eid);

    auto rc = pldm_send(eid, fd(), request, requestMsg.size());
    handlePLDMSendFailure(rc);
    lg2::info("Done. PLDM message, id: {ID}, RC: {RC}", "ID", id, "RC", rc);
}

void HostTransport::requestDelete(uint32_t dumpId)
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

    std::array<uint8_t, PLDM_MSG_HDR_SIZE + PLDM_FILE_ACK_REQ_BYTES>
        fileAckReqMsg;
    auto request = reinterpret_cast<pldm_msg*>(fileAckReqMsg.data());

    mctp_eid_t mctpEndPointId = readEID();
    createFileAckReqMsg(request, pldmDumpType, dumpId, mctpEndPointId);

    CustomFd pldmFd(phosphor::dump::pldm::openPLDM());

    auto retCode = pldm_send(mctpEndPointId, pldmFd(), fileAckReqMsg.data(),
                             fileAckReqMsg.size());

    handlePLDMSendFailure(retCode);

    lg2::info(
        "Sent request to host to delete the dump, SRC_DUMP_ID: {SRC_DUMP_ID}",
        "SRC_DUMP_ID", dumpId);
}

} // namespace host
} // namespace dump
} // namespace phosphor
