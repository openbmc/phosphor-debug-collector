# SPDX-License-Identifier: Apache-2.0

project('phosphor-debug-collector',
        'cpp',
        default_options: [
                          'cpp_std=c++17',
                          'warning_level=3',
                          'werror=true'
                         ],
        version: '1.0',
        license: 'Apache-2.0'
       )

# Checking dependency external library

cppfs = meson.get_compiler('cpp').find_library('stdc++fs')
libsystemd = dependency('libsystemd', version : '>=221')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
sdbusplus = dependency('sdbusplus')
phosphor_logging = dependency('phosphor-logging')

# Configuration header file(config.h) generation

conf_data = configuration_data()

conf_data.set_quoted('DUMP_BUSNAME', get_option('DUMP_BUSNAME'),
                      description : 'The Dbus busname to own'
                    )
conf_data.set_quoted('DUMP_OBJPATH', get_option('DUMP_OBJPATH'),
                      description : 'The Dump manager Dbus root'
                    )
conf_data.set_quoted('CORE_FILE_DIR', get_option('CORE_FILE_DIR'),
                      description : 'Directory where core dumps are placed'
                    )
conf_data.set_quoted('OBJ_INTERNAL', get_option('OBJ_INTERNAL'),
                      description : 'Internal Dump manager Dbus object path'
                    )
conf_data.set_quoted('OBJ_ENTRY', get_option('OBJ_ENTRY'),
                      description : 'The dump entry DBus object path'
                    )
conf_data.set_quoted('BMC_DUMP_PATH', get_option('BMC_DUMP_PATH'),
                     description : 'Directory where bmc dumps are placed')
conf_data.set('BMC_DUMP_MAX_SIZE', get_option('BMC_DUMP_MAX_SIZE'),
               description : 'Maximum size of one bmc dump in kilo bytes'
             )
conf_data.set('BMC_DUMP_MIN_SPACE_REQD', get_option('BMC_DUMP_MIN_SPACE_REQD'),
               description : 'Minimum space required for one bmc dump in kilo bytes'
             )
conf_data.set('BMC_DUMP_TOTAL_SIZE', get_option('BMC_DUMP_TOTAL_SIZE'),
               description : 'Total size of the dump in kilo bytes'
             )
conf_data.set_quoted('OBJ_LOGGING', '/xyz/openbmc_project/logging',
                      description : 'The log manager DBus object path'
                    )
conf_data.set_quoted('ELOG_ID_PERSIST_PATH', get_option('ELOG_ID_PERSIST_PATH'),
                      description : 'Path of file for storing elog id\'s, which have associated dumps'
                    )
conf_data.set('CLASS_VERSION', get_option('CLASS_VERSION'),
               description : 'Class version to register with Cereal'
             )
conf_data.set('ERROR_MAP_YAML', get_option('ERROR_MAP_YAML'),
               description : 'YAML filepath containing error object paths'
             )
conf_data.set('UBI_CORE_FILE_WORKAROUND', get_option('ubifs-workaround').enabled(),
               description : 'Turn on ubi workaround for core file'
             )

configure_file(configuration : conf_data,
               output : 'config.h'
              )

subdir('xyz/openbmc_project/Dump/Internal/Create')

python = find_program('python3')
errors_map_gen_file_loc = meson.source_root()
errors_map_gen_file_loc += '/errors_map_gen.py'

errors_map_hpp = custom_target(
                    'errors_map.hpp',
                    command : [
                        python,
                        errors_map_gen_file_loc,
                        '-i',
                        get_option('ERROR_MAP_YAML')
                    ],
                    depend_files : [ 'errors_map.mako.hpp',
                                     'errors_map_gen.py',
                                     get_option('ERROR_MAP_YAML')
                                   ],
                    output : 'errors_map.hpp'
                 )

phosphor_dump_manager_sources = [
        'dump_entry.cpp',
        'dump_manager.cpp',
        'dump_manager_main.cpp',
        'dump_serialize.cpp',
        'elog_watch.cpp',
        errors_map_hpp,
        server_hpp,
        server_cpp,
        'watch.cpp',
        'bmc_dump_entry.cpp',
        'dump_utils.cpp',
        'system_dump_entry.cpp',
        'dump_offload.cpp'
    ]

phosphor_dump_manager_dependency = [
        phosphor_dbus_interfaces,
        sdbusplus,
        phosphor_logging,
        cppfs
    ]

phosphor_dump_manager_install = true

# To get host dump offload transport source files and dependency list
# for phosphor_dump_manager
subdir('offload-extensions')

phosphor_dump_monitor_sources = [
        'core_manager.cpp',
        'core_manager_main.cpp',
        'watch.cpp'
    ]

phosphor_dump_monitor_dependency = [
        phosphor_dbus_interfaces,
        phosphor_logging,
        cppfs
    ]

phosphor_dump_monitor_install = true

executables = [[ 'phosphor-dump-manager',
                  phosphor_dump_manager_sources,
                  phosphor_dump_manager_dependency,
                  phosphor_dump_manager_install
               ],
               [ 'phosphor-dump-monitor',
                  phosphor_dump_monitor_sources,
                  phosphor_dump_monitor_dependency,
                  phosphor_dump_monitor_install
               ]
              ]

foreach executable : executables
    binary = executable(
                        executable[0],
                        executable[1],
                        dependencies: executable[2],
                        install : executable[3]
                       )
endforeach

if get_option('tests').enabled()
  subdir('test')
endif
