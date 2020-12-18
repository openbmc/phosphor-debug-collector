#include <cstdint>
#include <stdexcept>

namespace phosphor
{
namespace dump
{
namespace host
{

void requestOffload(uint32_t id)
{
    throw std::runtime_error("Hostdump offload method not specified, id = %d", id);
}

void requestDelete(uint32_t id, uint32_t dumpType)
{
    throw std::runtime_error("Hostdump delete method not specified, id = %d, dump type  = %d", id, dumpType);
}
} // namespace host
} // namespace dump
} // namespace phosphor
