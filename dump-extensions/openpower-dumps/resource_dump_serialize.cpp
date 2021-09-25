#include "config.h"

#include "resource_dump_serialize.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <phosphor-logging/log.hpp>

#include <fstream>

// Register class version
// From cereal documentation;
// "This macro should be placed at global scope"
CEREAL_CLASS_VERSION(openpower::dump::resource::Entry, CLASS_VERSION);

namespace openpower
{
namespace dump
{
namespace resource
{

/** @brief Function required by Cereal to perform serialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] a       - reference to Cereal archive.
 *  @param[in] e       - const reference to entry.
 *  @param[in] version - Class version that enables handling
 *                       a serialized data across code levels
 */
template <class Archive>
void save(Archive& a, const openpower::dump::resource::Entry& e,
          uint32_t /*version*/)
{
    a(e.getID(), e.elapsed(), e.completedTime(), e.size(), e.sourceDumpId(),
      e.vspString(), e.password(), e.status());
}

/** @brief Function required by Cereal to perform deserialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] a       - reference to Cereal archive.
 *  @param[in] e       - reference to error entry.
 *  @param[in] version - Class version that enables handling
 *                       a serialized data across code levels
 */
template <class Archive>
void load(Archive& a, openpower::dump::resource::Entry& e, uint32_t /*version*/)
{
    uint32_t dumpId{};
    uint64_t elapsed{};
    uint64_t completedTime{};
    uint64_t dumpSize{};
    uint32_t sourceId{};
    std::string vspStr{};
    std::string pwd{};
    phosphor::dump::OperationStatus status{};

    a(dumpId, elapsed, completedTime, dumpSize, sourceId, vspStr, pwd, status);

    e.setID(dumpId);
    e.elapsed(elapsed, true);
    e.completedTime(completedTime, true);
    e.size(dumpSize);
    e.sourceDumpId(sourceId, true);
    e.vspString(vspStr);
    e.password(pwd);
    e.status(status, true);
}

fs::path serialize(const openpower::dump::resource::Entry& e,
                   const fs::path& dir)
{
    auto path = dir / std::to_string(e.getID());
    std::ofstream os(path.c_str(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(e);
    return path;
}

bool deserialize(const fs::path& path, openpower::dump::resource::Entry& e)
{
    using namespace phosphor::logging;
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::BinaryInputArchive iarchive(is);
            iarchive(e);
            return true;
        }
        return false;
    }
    catch (cereal::Exception& e)
    {
        log<level::ERR>(e.what());
        fs::remove(path);
        return false;
    }
    catch (const std::length_error& e)
    {
        log<level::ERR>(e.what());
        fs::remove(path);
        return false;
    }
}
} // namespace resource
} // namespace dump
} // namespace openpower
