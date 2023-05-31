// SPDX-License-Identifier: Apache-2.0

#include "pldm_utils.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>

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
        log<level::ERR>(
            fmt::format("pldm_open failed, errno({}), FD({})", e, fd).c_str());
        elog<NotAllowed>(Reason("Required host dump action via pldm is not "
                                "allowed due to pldm_open failed"));
    }
    return fd;
}

uint8_t getPLDMInstanceID(uint8_t eid)
{
    constexpr auto pldmRequester = "xyz.openbmc_project.PLDM.Requester";
    constexpr auto pldm = "/xyz/openbmc_project/pldm";
    uint8_t instanceID = 0;

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto service = "xyz.openbmc_project.PLDM";

        auto method = bus.new_method_call(service, pldm, pldmRequester,
                                          "GetInstanceId");
        method.append(eid);
        auto reply = bus.call(method);

        reply.read(instanceID);

        log<level::INFO>(
            fmt::format("Got instanceId({}) from PLDM eid({})", instanceID, eid)
                .c_str());
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>(
            fmt::format("Failed to get instance id error({})", e.what())
                .c_str());
        elog<NotAllowed>(Reason("Failure in communicating with pldm service, "
                                "service may not be running"));
    }
    return instanceID;
}

} // namespace pldm
} // namespace dump
} // namespace phosphor
