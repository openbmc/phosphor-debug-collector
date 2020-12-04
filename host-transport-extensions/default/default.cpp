#include <cstdint>
#include <stdexcept>

namespace phosphor
{
namespace dump
{
namespace host
{
void requestOffload(uint32_t)
{
    throw std::runtime_error("Hostdump offload method not specified");
}

void requestDelete(uint32_t)
{
    throw std::runtime_error("Hostdump delete method not specified");
}
} // namespace host
} // namespace dump
} // namespace phosphor
