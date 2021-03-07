# SPDX-License-Identifier: Apache-2.0

# Configuration header file(openpower_dumps_config.h) generation
opconf_data = configuration_data()

opconf_data.set_quoted('SYSTEM_DUMP_OBJPATH', get_option('SYSTEM_DUMP_OBJPATH'),
                      description : 'The system dump manager D-Bus path'
                    )
opconf_data.set_quoted('SYSTEM_DUMP_OBJ_ENTRY', get_option('SYSTEM_DUMP_OBJ_ENTRY'),
                      description : 'The system dump entry D-Bus object path'
                    )

opconf_data.set_quoted('RESOURCE_DUMP_OBJPATH', get_option('RESOURCE_DUMP_OBJPATH'),
                      description : 'The resource dump manager D-Bus path'
                    )
opconf_data.set_quoted('RESOURCE_DUMP_OBJ_ENTRY', get_option('RESOURCE_DUMP_OBJ_ENTRY'),
                      description : 'The resource dump entry D-Bus object path'
                    )


opconf_data.set_quoted('HOSTBOOT_DUMP_OBJPATH', get_option('HOSTBOOT_DUMP_OBJPATH'),
                      description : 'The Hostboot dump manager D-Bus path'
                    )
opconf_data.set_quoted('HOSTBOOT_DUMP_OBJ_ENTRY', get_option('HOSTBOOT_DUMP_OBJ_ENTRY'),
                      description : 'The hostboot dump entry D-Bus object path'
                    )
opconf_data.set_quoted('HOSTBOOT_DUMP_TMP_FILE_DIR', get_option('HOSTBOOT_DUMP_TMP_FILE_DIR'),
                      description : 'Directory where hostboot dump pieces are stored for packaging'
                    )
opconf_data.set_quoted('HOSTBOOT_DUMP_PATH', get_option('HOSTBOOT_DUMP_PATH'),
                     description : 'Directory where Hostboot dumps are placed'
             )
opconf_data.set('HOSTBOOT_DUMP_MAX_SIZE', get_option('HOSTBOOT_DUMP_MAX_SIZE'),
               description : 'Maximum size of one Hostboot dump in kilo bytes'
             )
opconf_data.set('HOSTBOOT_DUMP_MIN_SPACE_REQD', get_option('HOSTBOOT_DUMP_MIN_SPACE_REQD'),
               description : 'Minimum space required for one Hostboot dump in kilo bytes'
             )
opconf_data.set('HOSTBOOT_DUMP_TOTAL_SIZE', get_option('HOSTBOOT_DUMP_TOTAL_SIZE'),
               description : 'Total size of the dump in kilo bytes'
             )

configure_file(configuration : opconf_data,
               output : 'openpower_dumps_config.h'
              )

phosphor_dump_manager_sources += [
        'dump-extensions/openpower-dumps/dump-extensions.cpp',
        'dump-extensions/openpower-dumps/dump_manager_system.cpp',
        'dump-extensions/openpower-dumps/system_dump_entry.cpp',
        'dump-extensions/openpower-dumps/dump_manager_resource.cpp',
        'dump-extensions/openpower-dumps/resource_dump_entry.cpp',
        'dump-extensions/openpower-dumps/op_dump_util.cpp',
    ]
