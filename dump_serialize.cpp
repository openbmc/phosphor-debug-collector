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
/** @brief Function required by Cereal to perform serialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] a - reference to Cereal archive.
 *  @param[in] e - const reference to elog id list.
 */
template<class Archive>
void save(Archive& a, const ElogList& e)
{
    a(e);
}

/** @brief Function required by Cereal to perform deserialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] a - reference to Cereal archive.
 *  @param[in] l - reference to elog id list.
 */
template<class Archive>
void load(Archive& a, ElogList& e)
{
    a(e);
}

void serialize(const ElogList& l, const fs::path& dir)
{
    std::ofstream os(dir.c_str(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(l);
}

bool deserialize(const fs::path& path, ElogList& l)
{
    if (fs::exists(path))
    {
        std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
        cereal::BinaryInputArchive iarchive(is);
        iarchive(l);
        return true;
    }
    return false;
}

} // namespace elog
} // namespace dump
} // namespace phosphor
