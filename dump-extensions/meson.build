# SPDX-License-Identifier: Apache-2.0

clangtidy = find_program('clang-tidy', required: false)
if get_option('openpower-dumps-extension').allowed() or clangtidy.found()
    subdir('openpower-dumps')
else
    subdir('default')
endif
