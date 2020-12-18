#include <cstdint>
#include <stdexcept>

namespace phosphor
{
namespace dump
{
namespace host
{

enum class HostDumpType
{
    defaultType, // The default dump type.
};

void requestOffload(uint32_t)
{
    throw std::runtime_error("Hostdump offload method not specified");
}

void requestDelete(uint32_t, HostDumpType dumpType)
{
    throw std::runtime_error("Hostdump delete method not specified dump type  = %d", dumpType);
}
} // namespace host
} // namespace dump
} // namespace phosphor
