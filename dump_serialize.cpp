#include "dump_serialize.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/types/set.hpp>
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/log.hpp>

#include <fstream>

namespace phosphor
{
namespace dump
{
namespace elog
{

using namespace phosphor::logging;

void serialize(const ElogList& list, const std::filesystem::path& dir)
{
    std::ofstream os(dir.c_str(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(list);
}

bool deserialize(const std::filesystem::path& path, ElogList& list)
{
    try
    {
        if (std::filesystem::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::BinaryInputArchive iarchive(is);
            iarchive(list);
            return true;
        }
        return false;
    }
    catch (const cereal::Exception& e)
    {
        lg2::error("Failed to deserialize, errormsg: {ERROR}", "ERROR", e);
        std::filesystem::remove(path);
        return false;
    }
}

} // namespace elog
} // namespace dump
} // namespace phosphor
