#include "dump-extensions/openpower-dumps/openpower_dumps_config.h"

#include "system_dump_entry.hpp"

#include <filesystem>

namespace openpower
{
namespace dump
{
namespace system
{
namespace fs = std::filesystem;

/** @brief Serialize and persist dump d-bus object
 *  @param[in] a - const reference to entry.
 *  @param[in] dir - pathname of directory where the serialized system dump
 *                   entries will be placed.
 *  @return fs::path - pathname of persisted dump entry file
 */
fs::path serialize(const openpower::dump::system::Entry& e,
                   const fs::path& dir = fs::path(SYSTEM_DUMP_SERIAL_PATH));

/** @brief Deserialze a persisted dump d-bus object
 *  @param[in] path - pathname of persisted entry file
 *  @param[in] e - reference to dump object which is the target of
 *             deserialization.
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, openpower::dump::system::Entry& e);
} // namespace system
} // namespace dump
} // namespace openpower
