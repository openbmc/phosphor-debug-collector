# SPDX-License-Identifier: Apache-2.0

phosphor_dump_manager_sources += [
        'host-transport-extensions/pldm/common/pldm_utils.cpp'
    ]

phosphor_dump_manager_dependency += [ dependency('libpldm') ]

if get_option('openpower-dumps-extension').enabled()
    subdir('oem/ibm')
else
    subdir('default')
endif
