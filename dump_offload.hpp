#pragma once

#include <filesystem>

namespace phosphor
{
namespace dump
{
namespace offload
{

/**
 * @brief Kicks off the instructions to
 *        start offload of the dump using dbus
 *
 * @param[in] file - dump filename with relative path.
 * @param[in] dumpId - id of the dump.
 * @param[in] writePath[in] - path to write the dump file.
 *
 **/
void requestOffload(std::filesystem::path file, uint32_t dumpId,
                    std::string writePath);

} // namespace offload
} // namespace dump
} // namespace phosphor
