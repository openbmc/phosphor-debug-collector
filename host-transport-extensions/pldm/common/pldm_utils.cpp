// SPDX-License-Identifier: Apache-2.0

#include "pldm_utils.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace pldm
{

using namespace phosphor::logging;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

int openPLDM()
{
    auto fd = pldm_open();
    if (fd < 0)
    {
        auto e = errno;
        log<level::ERR>("pldm_open failed", entry("ERRNO=%d", e),
                        entry("FD=%d\n", fd));
        elog<NotAllowed>(Reason("Required host dump action via pldm is not "
                                "allowed due to pldm_open failed"));
    }
    return fd;
}

uint8_t getPLDMInstanceID(uint8_t eid)
{

    constexpr auto pldmRequester = "xyz.openbmc_project.PLDM.Requester";
    constexpr auto pldm = "/xyz/openbmc_project/pldm";

    auto bus = sdbusplus::bus::new_default();
    auto service = phosphor::dump::getService(bus, pldm, pldmRequester);

    auto method = bus.new_method_call(service.c_str(), pldm, pldmRequester,
                                      "GetInstanceId");
    method.append(eid);
    auto reply = bus.call(method);

    uint8_t instanceID = 0;
    reply.read(instanceID);

    return instanceID;
}

} // namespace pldm
} // namespace dump
} // namespace phosphor
