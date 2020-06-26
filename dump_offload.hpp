#pragma once

#include "config.h"

#include "dump_entry.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"
#include "xyz/openbmc_project/Dump/Entry/System/server.hpp"

#include <fcntl.h>

#include <filesystem>
#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

namespace phosphor
{
namespace dump
{
namespace offload
{

namespace fs = std::filesystem;

/**
 * @brief Kicks off the instructions to
 *        start offload of the dump using dbus
 *
 * @param[in] file - dump filename with relative path.
 * @param[in] dumpId - id of the dump.
 * writePath[in] - path to write the dump file.
 *
 **/
void requestOffload(fs::path file, uint32_t dumpId, std::string writePath);

} // namespace offload
} // namespace dump
} // namespace phosphor
