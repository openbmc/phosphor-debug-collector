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

} // namespace host
} // namespace dump
} // namespace phosphor
