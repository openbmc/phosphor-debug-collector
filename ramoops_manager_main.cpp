#include "config.h"

#include "ramoops_manager.hpp"

#include <phosphor-logging/elog-errors.hpp>

int main()
{
    fs::path filePath(SYSTEMD_PSTORE_PATH);
    if (!fs::exists(filePath))
    {
        log<level::ERR>("Pstore file path is not exists",
                        entry("FILE_PATH = %s", SYSTEMD_PSTORE_PATH));
        return EXIT_FAILURE;
    }

    phosphor::dump::ramoops::Manager manager(SYSTEMD_PSTORE_PATH);

    return EXIT_SUCCESS;
}
