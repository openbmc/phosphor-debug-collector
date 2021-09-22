#include "system_dump_entry.hpp"

#include <filesystem>

namespace openpower
{
namespace dump
{
namespace system
{
namespace fs = std::filesystem;

/** @brief Serialize and persist error d-bus object
 *  @param[in] a - const reference to entry.
 *  @param[in] dir - pathname of directory where the serialized error will
 *                   be placed.
 *  @return fs::path - pathname of persisted error file
 */
fs::path serialize(const openpower::dump::system::Entry& e,
                   const fs::path& dir);

/** @brief Deserialze a persisted error into a d-bus object
 *  @param[in] path - pathname of persisted entry file
 *  @param[in] e - reference to error object which is the target of
 *             deserialization.
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, openpower::dump::system::Entry& e);
} // namespace system
} // namespace dump
} // namespace openpower
