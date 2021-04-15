# SPDX-License-Identifier: Apache-2.0

phosphor_dump_manager_sources += [
        'dump-extensions/openpower-dumps/dump-extensions.cpp',
        'dump-extensions/openpower-dumps/dump_manager_system.cpp',
        'dump-extensions/openpower-dumps/system_dump_entry.cpp',
        'dump-extensions/openpower-dumps/dump_manager_resource.cpp',
        'dump-extensions/openpower-dumps/resource_dump_entry.cpp',
        'dump-extensions/openpower-dumps/dump_manager_hostboot.cpp',
        'dump-extensions/openpower-dumps/hostboot_dump_entry.cpp'
    ]

unit_files = [
        'create_openpower_dump_dirs.service'
   ]

unit_subs = configuration_data()
unit_subs.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))
systemd_system_unit_dir = dependency('systemd').get_pkgconfig_variable(
    'systemdsystemunitdir',
    define_variable: ['prefix', get_option('prefix')])
foreach u : unit_files
    configure_file(
        configuration: unit_subs,
        input: u + '.in',
        install: true,
        install_dir: systemd_system_unit_dir,
        output: u
    )
endforeach
