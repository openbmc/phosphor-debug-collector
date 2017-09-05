#include <cereal/types/set.hpp>
#include <cereal/archives/binary.hpp>
#include <fstream>

#include "dump_serialize.hpp"

namespace phosphor
{
namespace dump
{
namespace elog
{

void serialize(const ElogList& list, const fs::path& dir)
{
    std::ofstream os(dir.c_str(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(list);
}

bool deserialize(const fs::path& path, ElogList& list)
{
    if (fs::exists(path))
    {
        std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
        cereal::BinaryInputArchive iarchive(is);
        iarchive(list);
        return true;
    }
    return false;
}

} // namespace elog
} // namespace dump
} // namespace phosphor
