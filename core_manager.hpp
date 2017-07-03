#pragma once

#include <map>

#include "dump_utils.hpp"
#include "watch.hpp"

namespace phosphor
{
namespace dump
{
namespace core
{
namespace manager
{

using UserMap = phosphor::dump::inotify::UserMap;

/** @brief Implementation of core watch call back
  * @param [in] fileInfo - map of file info  path:event
  */
void watchCallback(const UserMap& fileInfo);

} // namespace manager
} // namepsace core
} // namespace dump
} // namespace phosphor
