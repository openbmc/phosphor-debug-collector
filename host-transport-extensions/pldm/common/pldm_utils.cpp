// SPDX-License-Identifier: Apache-2.0

#include "pldm_utils.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

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

int openPLDM()
{
    auto fd = pldm_open();
    if (fd < 0)
    {
        auto e = errno;
        lg2::error(
            "pldm_open failed, errno: {ERRNO}, FD: FD", "ERRNO", e, "FD",
            static_cast<std::underlying_type<pldm_requester_error_codes>::type>(
                fd));
        elog<NotAllowed>(
            Reason("Required host dump action via pldm is not allowed due "
                   "to pldm_open failed"));
    }
    return fd;
}

} // namespace pldm
} // namespace dump
} // namespace phosphor
