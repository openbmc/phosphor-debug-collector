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
void watchCallback(UserMap fileInfo);

/** @brief Helper function for initiating dump request using
 *         D-bus internal create interface.
 *         @param [in] files - Core files list
 */
void createHelper(std::vector<std::string>& files);

} // namespace manager
} // namepsace core
} // namespace dump
} // namespace phosphor
