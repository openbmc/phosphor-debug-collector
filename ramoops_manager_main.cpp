#include "config.h"

#include "ramoops_manager.hpp"

#include <fmt/core.h>

#include <phosphor-logging/elog-errors.hpp>

int main()
{
    fs::path filePath(SYSTEMD_PSTORE_PATH);
    if (!fs::exists(filePath))
    {
        log<level::ERR>(
            fmt::format("Pstore file path is not exists, FILE_PATH({})",
                        SYSTEMD_PSTORE_PATH)
                .c_str());
        return EXIT_FAILURE;
    }

    phosphor::dump::ramoops::Manager manager(SYSTEMD_PSTORE_PATH);

    return EXIT_SUCCESS;
}
