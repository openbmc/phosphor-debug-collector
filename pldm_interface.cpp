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

#include <fstream>
#include <phosphor-logging/log.hpp>

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

// constexpr uint16_t pelFileType = 0;

PLDMInterface::~PLDMInterface()
{
    closeFD();
}

void PLDMInterface::closeFD()
{
    if (_fd >= 0)
    {
        close(_fd);
        _fd = -1;
    }
}

void PLDMInterface::readEID()
{
    _eid = defaultEIDValue;

    std::ifstream eidFile{eidPath};
    if (!eidFile.good())
    {
        log<level::ERR>("Could not open host EID file");
    }
    else
    {
        std::string eid;
        eidFile >> eid;
        if (!eid.empty())
        {
            _eid = atoi(eid.c_str());
        }
        else
        {
            log<level::ERR>("EID file was empty");
        }
    }
}

void PLDMInterface::open()
{
    _fd = pldm_open();
    if (_fd < 0)
    {
        auto e = errno;
        log<level::ERR>("pldm_open failed", entry("ERRNO=%d", e),
                        entry("RC=%d\n", _fd));
        throw std::exception{};
    }
}

CmdStatus PLDMInterface::sendGetSysDumpCmd(uint32_t id)
{
    try
    {
        closeFD();

        open();

        readInstanceID();

        // registerReceiveCallback();

        doSendRcv(id);
    }
    catch (const std::exception& e)
    {
        closeFD();

        _inProgress = false;
        return CmdStatus::failure;
    }

    _inProgress = true;
    return CmdStatus::success;
}

void PLDMInterface::readInstanceID()
{
    try
    {
        _instanceID = getPLDMInstanceID(_eid);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            "Failed to get instance ID from PLDM Requester D-Bus daemon",
            entry("ERROR=%s", e.what()));
        throw;
    }
}

void PLDMInterface::doSendRcv(uint32_t id)
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
        throw std::exception{};
    }

    uint8_t* responseMsg = nullptr;
    size_t responseMsgSize{};

    rc = pldm_send_recv(_eid, _fd, requestMsg.data(), requestMsg.size(),
                        &responseMsg, &responseMsgSize);
    if (rc < 0)
    {
        auto e = errno;
        log<level::ERR>("pldm_send failed", entry("RC=%d", rc),
                        entry("ERRNO=%d", e));

        throw std::exception{};
    }
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg);
    log<level::INFO>(
        "Done. PLDM message",
        entry("RC=%d", static_cast<uint16_t>(response->payload[0])));
}

uint8_t PLDMInterface::getPLDMInstanceID(uint8_t eid) const
{
    return 0;
// Don't use until PLDM switches to async D-Bus
#if 0                                                                           
    auto service = getService(object_path::pldm, interface::pldmRequester);     
                                                                                
    auto method =                                                               
        _bus.new_method_call(service.c_str(), object_path::pldm,                
                             interface::pldmRequester, "GetInstanceId");        
    method.append(eid);                                                         
    auto reply = _bus.call(method);                                             
                                                                                
    uint8_t instanceID = 0;                                                     
    reply.read(instanceID);                                                     
                                                                                
    return instanceID;
#endif
}

} // namespace pldm
} // namespace dump
} // namespace phosphor
