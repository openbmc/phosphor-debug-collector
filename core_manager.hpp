#pragma once

#include <map>

#include "dump_utils.hpp"

namespace phosphor
{
namespace dump
{
namespace core
{
/** @class Manager
 *
 *  @brief Manages the acttivation of dump collection for the coredump.
 *
 */
class Manager
{
    public:
        Manager() = default;
        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;
        Manager(Manager&&) = delete;
        Manager& operator=(Manager&&) = delete;
        ~Manager() = default;

        /** @brief Implementation of core watch call back
          * @param [out] fileInfo - map of file info  name:event
          */
        static void watchCallback(std::map<fs::path, uint32_t> fileInfo);
};

} // namepsace core
} // namespace dump
} // namespace phosphor
