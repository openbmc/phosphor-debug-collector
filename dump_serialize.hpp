#pragma once

#include <experimental/filesystem>
#include "elog_watch.hpp"
#include "config.h"

namespace phosphor
{
namespace dump
{
namespace elog
{

namespace fs = std::experimental::filesystem;

/** @brief Serialize and persist elog id.
 *  @param[in] e - elog id list.
 *  @param[in] dir - pathname of file where the serialized elog id's will
 *                   be placed.
 */
void serialize(const ElogList& e,
                   const fs::path& dir = fs::path(ELOG_ID_PERSIST_PATH));

/** @brief Deserialze a persisted elog id into list
 *  @param[in] path - pathname of persisted error file
 *  @param[in] e - elog id list
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, ElogList& e);

} // namespace elog
} // namespace dump
} // namespace phosphor
