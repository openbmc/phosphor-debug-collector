# SPDX-License-Identifier: Apache-2.0

project('phosphor-debug-collector',
        'cpp',
        meson_version: '>=1.1.1',
        default_options: [
                          'cpp_std=c++23',
                          'warning_level=3',
                          'werror=true',
                          'buildtype=debugoptimized'
                         ],
        version: '1.0',
        license: 'Apache-2.0'
       )

cpp = meson.get_compiler('cpp')

# list of unit files, the path as input and service name
# as output
# eg: unit_file += {'input:'<path>, 'output':<service name>}
unit_files = []

# Checking dependency external library

libsystemd = dependency('libsystemd', version : '>=221')

sdbusplus_dep = dependency('sdbusplus')
sdbusplusplus_prog = find_program('sdbus++')
sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson')
sdeventplus_dep = dependency('sdeventplus')

phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')

# Get Cereal dependency.
cereal_dep = dependency('cereal', required: false)
has_cereal = cpp.has_header_symbol(
    'cereal/cereal.hpp',
    'cereal::specialize',
    dependencies: cereal_dep,
    required: false)
if not has_cereal
    cereal_opts = import('cmake').subproject_options()
    cereal_opts.add_cmake_defines({'BUILD_TESTS': 'OFF'})
    cereal_proj = import('cmake').subproject(
        'cereal',
        options: cereal_opts,
        required: false)
    assert(cereal_proj.found(), 'cereal is required')
    cereal_dep = cereal_proj.dependency('cereal')
endif

# Disable FORTIFY_SOURCE when compiling with no optimization
if(get_option('optimization') == '0')
  add_project_arguments('-U_FORTIFY_SOURCE',language:['cpp','c'])
  message('Disabling FORTIFY_SOURCE as optimization is set to 0')
endif

# Configuration header file(config.h) generation

conf_data = configuration_data()

conf_data.set_quoted('DUMP_BUSNAME', get_option('DUMP_BUSNAME'),
                      description : 'The Dbus busname to own'
                    )
conf_data.set_quoted('DUMP_OBJPATH', get_option('DUMP_OBJPATH'),
                      description : 'The Dump manager Dbus root'
                    )
conf_data.set_quoted('BMC_DUMP_OBJPATH', get_option('BMC_DUMP_OBJPATH'),
                      description : 'The BMC Dump manager Dbus path'
                    )
conf_data.set_quoted('CORE_FILE_DIR', get_option('CORE_FILE_DIR'),
                      description : 'Directory where core dumps are placed'
                    )
conf_data.set_quoted('OBJ_INTERNAL', get_option('OBJ_INTERNAL'),
                      description : 'Internal Dump manager Dbus object path'
                    )
conf_data.set_quoted('BMC_DUMP_OBJ_ENTRY', get_option('BMC_DUMP_OBJ_ENTRY'),
                      description : 'The BMC dump entry DBus object path'
                    )
conf_data.set_quoted('BMC_DUMP_PATH', get_option('BMC_DUMP_PATH'),
                     description : 'Directory where bmc dumps are placed')
conf_data.set_quoted('SYSTEMD_PSTORE_PATH', get_option('SYSTEMD_PSTORE_PATH'),
                     description : 'Path to the systemd pstore directory')
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
conf_data.set('JFFS_CORE_FILE_WORKAROUND', get_option('jffs-workaround').enabled(),
               description : 'Turn on jffs workaround for core file'
             )
conf_data.set_quoted('FAULTLOG_DUMP_OBJ_ENTRY', get_option('FAULTLOG_DUMP_OBJ_ENTRY'),
                      description : 'The Fault Log dump entry DBus object path'
                    )
conf_data.set_quoted('FAULTLOG_DUMP_OBJPATH', get_option('FAULTLOG_DUMP_OBJPATH'),
                      description : 'The Fault Log Dump manager Dbus path'
                    )
conf_data.set_quoted('FAULTLOG_DUMP_PATH', get_option('FAULTLOG_DUMP_PATH'),
                      description : 'Directory where fault logs are placed'
                    )
conf_data.set('BMC_DUMP_ROTATE_CONFIG', get_option('dump_rotate_config').enabled(),
               description : 'Turn on rotate config for bmc dump'
             )
conf_data.set_quoted('BMC_DUMP_FILENAME_REGEX', get_option('BMC_DUMP_FILENAME_REGEX'),
                      description: 'BMC Dump filename format'
            )
conf_data.set('FILENAME_DUMP_ID_POS', get_option('FILENAME_DUMP_ID_POS'),
               description : 'Position of dump id in the dump filename'
             )
conf_data.set('FILENAME_EPOCHTIME_POS', get_option('FILENAME_EPOCHTIME_POS'),
               description : 'Position of timestamp in the dump filename'
             )
conf_data.set('TIMESTAMP_FORMAT', get_option('TIMESTAMP_FORMAT'),
               description : 'Timestamp format in filename: 0-epoch 1-human readable'
             )

configure_file(configuration : conf_data,
               output : 'config.h'
              )

dump_types_yaml_files = []

# Dump types YAML file
dump_types_yaml_files += {'input': 'example_dump_types.yaml',
         'output': 'dump_types.yaml'}

# Copy and combine YAML files
concatenate_command = 'cat '
combined_yaml_file = 'combined_dump_types.yaml'

foreach yaml_file : dump_types_yaml_files
    configure_file(input : yaml_file.get('input'),
                   output : yaml_file.get('output'),
                   copy : true)
    concatenate_command += meson.build_root() + '/' + yaml_file.get('output') + ' '
endforeach

concatenate_command += '> ' + meson.build_root() + '/' + combined_yaml_file
run_command('sh', '-c', concatenate_command)

python = find_program('python3')
map_gen_file_loc = meson.project_source_root()
map_gen_file_loc += '/map_gen.py'

dump_types_hpp = custom_target(
                    'dump_types.hpp',
                    command : [
                        python,
                        map_gen_file_loc,
                        '-i',
                        meson.build_root() + '/' +  combined_yaml_file,
                        '-t',
                        'dump_types.mako.hpp',
                        '-v',
                        'DUMP_TYPE_TABLE',
                        '-o',
                        'dump_types.hpp'
                    ],
                    depend_files : [ 'dump_types.mako.hpp',
                                     'map_gen.py',
                                     meson.build_root() + '/' +  combined_yaml_file
                                   ],
                    output : 'dump_types.hpp'
                 )

dump_types_cpp = custom_target(
                    'dump_types.cpp',
                    command : [
                        python,
                        map_gen_file_loc,
                        '-i',
                        meson.build_root() + '/' +  combined_yaml_file,
                        '-t',
                        'dump_types.mako.cpp',
                        '-v',
                        'DUMP_TYPE_TABLE',
                        '-o',
                        'dump_types.cpp'
                    ],
                    depend_files : [ 'dump_types.mako.cpp',
                                     'map_gen.py',
                                     meson.build_root() + '/' +  combined_yaml_file
                                   ],
                    output : 'dump_types.cpp'
                 )

errors_map_cpp = custom_target(
                    'errors_map.cpp',
                    command : [
                        python,
                        map_gen_file_loc,
                        '-i',
                        get_option('ERROR_MAP_YAML'),
                        '-t',
                        'errors_map.mako.cpp',
                        '-v',
                        'errDict',
                        '-o',
                        'errors_map.cpp'
                    ],
                    depend_files : [ 'errors_map.mako.cpp',
                                     'map_gen.py',
                                     get_option('ERROR_MAP_YAML')
                                   ],
                    output : 'errors_map.cpp'
                 )

phosphor_dump_manager_sources = [
        'dump_entry.cpp',
        'base_dump_entry.cpp',
        'dump_manager.cpp',
        'dump_manager_bmc.cpp',
        'dump_manager_main.cpp',
        'dump_serialize.cpp',
        'elog_watch.cpp',
        errors_map_cpp,
        dump_types_hpp,
        dump_types_cpp,
        'watch.cpp',
        'dump_utils.cpp',
        'dump_offload.cpp',
        'dump_manager_faultlog.cpp',
        'faultlog_dump_entry.cpp'
    ]

phosphor_dump_manager_dependency = [
        phosphor_dbus_interfaces_dep,
        sdbusplus_dep,
        sdeventplus_dep,
        phosphor_logging_dep,
        cereal_dep,
    ]

phosphor_dump_manager_install = true

phosphor_dump_manager_incdir = []

# To get host transport based interface to take respective host
# dump actions. It will contain required sources and dependency
# list for phosphor_dump_manager.
subdir('host-transport-extensions')

#pick any architecture specific dumps
subdir('dump-extensions')

phosphor_dump_monitor_sources = [
        dump_types_hpp,
        'core_manager.cpp',
        'core_manager_main.cpp',
        'watch.cpp'
    ]

phosphor_dump_monitor_dependency = [
        phosphor_dbus_interfaces_dep,
        phosphor_logging_dep
    ]

phosphor_dump_monitor_install = true

phosphor_dump_monitor_incdir = []

phosphor_ramoops_monitor_sources = [
        'ramoops_manager.cpp',
        'ramoops_manager_main.cpp',
        'watch.cpp'
    ]

phosphor_ramoops_monitor_dependency = [
        phosphor_dbus_interfaces_dep,
        phosphor_logging_dep
    ]

phosphor_ramoops_monitor_install = true

phosphor_ramoops_monitor_incdir = []

executables = [[ 'phosphor-dump-manager',
                  phosphor_dump_manager_sources,
                  phosphor_dump_manager_dependency,
                  phosphor_dump_manager_install,
                  phosphor_dump_manager_incdir
               ],
               [ 'phosphor-dump-monitor',
                  phosphor_dump_monitor_sources,
                  phosphor_dump_monitor_dependency,
                  phosphor_dump_monitor_install,
                  phosphor_dump_monitor_incdir
               ],
               [ 'phosphor-ramoops-monitor',
                  phosphor_ramoops_monitor_sources,
                  phosphor_ramoops_monitor_dependency,
                  phosphor_ramoops_monitor_install,
                  phosphor_ramoops_monitor_incdir
               ]
              ]

foreach executable : executables
    binary = executable(
                        executable[0],
                        executable[1],
                        dependencies: executable[2],
                        install : executable[3],
                        include_directories : executable[4]
                       )
endforeach

unit_subs = configuration_data()
unit_subs.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))
systemd_system_unit_dir = dependency('systemd').get_variable(
    'systemdsystemunitdir',
    pkgconfig_define: ['prefix', get_option('prefix')])
foreach u : unit_files
    configure_file(
        configuration: unit_subs,
        input: u.get('input'),
        install: true,
        install_dir: systemd_system_unit_dir,
        output: u.get('output')
    )
endforeach

if get_option('tests').enabled()
  subdir('test')
endif
