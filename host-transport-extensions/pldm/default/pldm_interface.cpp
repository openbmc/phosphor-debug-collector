// SPDX-License-Identifier: Apache-2.0

#include "dump_utils.hpp"
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
void requestOffload(uint32_t)
{
    throw std::runtime_error("PLDM: Hostdump offload method not specified");
}

void requestDelete(uint32_t)
{
    throw std::runtime_error("PLDM: Hostdump delete method not specified");
}
} // namespace host
} // namespace dump
} // namespace phosphor
