// SPDX-License-Identifier: Apache-2.0

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
void requestOffload(uint32_t)
{
    throw std::runtime_error("PLDM: Hostdump offload method not specified");
}

void requestDelete(uint32_t, uint32_t)
{
    throw std::runtime_error("PLDM: Hostdump delete method not specified");
}
} // namespace host
} // namespace dump
} // namespace phosphor
