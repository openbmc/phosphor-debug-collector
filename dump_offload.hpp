#pragma once

#include "dump_entry.hpp"
#include "config.h"
#include "xyz/openbmc_project/Dump/Entry/System/server.hpp"

#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <fstream>

namespace phosphor
{
namespace dump
{
namespace offload
{

namespace fs = std::experimental::filesystem;

/**
* @brief Kicks off the instructions to
*        start offload of the dump using dbus
*
* @param[in] file - dump filename with relative path.
*
**/
void requestOffload(fs::path file);

} // namespace offload
} // namespace dump
} // namespace phosphor
