// SPDX-License-Identifier: Apache-2.0

#include "host_transport_exts.hpp"

#include <stdint.h>

#include <stdexcept>

namespace phosphor
{
namespace dump
{
namespace host
{
/**
 * @brief Initiate offload of the dump with provided dump source ID
 *
 * @param[in] id - The Dump Source ID.
 *
 */
void HostTransport::requestOffload(uint32_t)
{
    throw std::runtime_error("PLDM: Hostdump offload method not specified");
}

void HostTransport::requestDelete(uint32_t)
{
    throw std::runtime_error("PLDM: Hostdump delete method not specified");
}
} // namespace host
} // namespace dump
} // namespace phosphor
