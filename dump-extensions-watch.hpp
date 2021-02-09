#include "dump_utils.hpp"
#include "file_watch_manager.hpp"

#include <memory>
#include <vector>

namespace phosphor
{
namespace dump
{

using FileWatchManagerList =
    std::vector<std::unique_ptr<phosphor::dump::filewatch::Manager>>;
/**
 * @brief load file watch extensions
 *
 * @param[in] event -
 * @param[out] fileWatchList - List of file watch objects
 *
 */
void loadFileWatchList(const EventPtr& event,
                       FileWatchManagerList& fileWatchList);
} // namespace dump
} // namespace phosphor
