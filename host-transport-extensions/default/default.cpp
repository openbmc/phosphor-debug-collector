#include "host_transport_exts.hpp"

#include <cstdint>
#include <stdexcept>

namespace phosphor
{
namespace dump
{
namespace host
{
void HostTransport::requestOffload(uint32_t)
{
    throw std::runtime_error("Hostdump offload method not specified");
}

void HostTransport::requestDelete(uint32_t)
{
    throw std::runtime_error("Hostdump delete method not specified");
}
} // namespace host
} // namespace dump
} // namespace phosphor
