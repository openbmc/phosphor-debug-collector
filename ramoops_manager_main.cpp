#include "config.h"

#include "ramoops_manager.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>

int main()
{
    fs::path filePath(SYSTEMD_PSTORE_PATH);
    if (!fs::exists(filePath))
    {
        lg2::error("Pstore file path is not exists, FILE_PATH: {FILE_PATH}",
                   "FILE_PATH", SYSTEMD_PSTORE_PATH) return EXIT_FAILURE;
    }

    phosphor::dump::ramoops::Manager manager(SYSTEMD_PSTORE_PATH);

    return EXIT_SUCCESS;
}
