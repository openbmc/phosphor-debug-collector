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
#include "pldm_interface.hpp"

#include <libpldm/base.h>
#include <libpldm/file_io.h>
#include <libpldm/platform.h>
#include <libpldm/pldm.h>
#include <unistd.h>
#include <phosphor-logging/elog-errors.hpp>
#include "dump_utils.hpp"

#include <fstream>
#include <phosphor-logging/log.hpp>

#include "xyz/openbmc_project/Common/error.hpp"

namespace phosphor
{
namespace dump
{
namespace pldm
{

using namespace phosphor::logging;
using namespace sdeventplus;
using namespace sdeventplus::source;

constexpr auto eidPath = "/usr/share/pldm/host_eid";
constexpr mctp_eid_t defaultEIDValue = 9;

using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

void closeFD( int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

mctp_eid_t readEID()
{
     mctp_eid_t eid = defaultEIDValue;

    std::ifstream eidFile{eidPath};
    if (!eidFile.good())
    {
        log<level::ERR>("Could not open host EID file");
        elog<InternalFailure>();
    }
    else
    {
        std::string eid;
        eidFile >> eid;
        if (!eid.empty())
        {
            eid = atoi(eid.c_str());
        }
        else
        {
            log<level::ERR>("EID file was empty");
            elog<InternalFailure>();
        }
    }

    return eid;
}

int open()
{
    auto fd = pldm_open();
    if (fd < 0)
    {
        auto e = errno;
        log<level::ERR>("pldm_open failed", entry("ERRNO=%d", e),
                        entry("RC=%d\n", fd));
        elog<InternalFailure>();
    }
    return fd;
}

void sendGetSysDumpCmd(uint32_t id)
{
    uint16_t effecterId = 0x05; // TODO PhyP temporary Hardcoded value.

    std::array<uint8_t, sizeof(pldm_msg_hdr) + sizeof(effecterId) + sizeof(id) +
        sizeof(uint8_t)> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    std::array<uint8_t, sizeof(id)> effecterValue{};

    memcpy(effecterValue.data(), &id, sizeof(id));

    auto rc = encode_set_numeric_effecter_value_req(
        0, effecterId, PLDM_EFFECTER_DATA_SIZE_UINT32, effecterValue.data(),
        request, requestMsg.size() - sizeof(pldm_msg_hdr));

    if (rc != PLDM_SUCCESS)
    {
        log<level::ERR>("Message encode failure. ", entry("RC=%d", rc));
        elog<InternalFailure>();
    }

    uint8_t* responseMsg = nullptr;
    size_t responseMsgSize{};

    mctp_eid_t eid = readEID();

    auto fd =  open();

    rc = pldm_send_recv(eid, fd, requestMsg.data(), requestMsg.size(),
                        &responseMsg, &responseMsgSize);
    if (rc < 0)
    {
        closeFD(fd);
        auto e = errno;
        log<level::ERR>("pldm_send failed", entry("RC=%d", rc),
                        entry("ERRNO=%d", e));

        elog<InternalFailure>();
    }
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg);
    log<level::INFO>(
        "Done. PLDM message",
        entry("RC=%d", static_cast<uint16_t>(response->payload[0])));

    closeFD(fd);
}
} // namespace pldm
} // namespace dump
} // namespace phosphor
