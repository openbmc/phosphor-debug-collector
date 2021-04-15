# SPDX-License-Identifier: Apache-2.0

unit_files += [
        ['dump-extensions/openpower-dumps/create_openpower_dump_dirs.service',
         'create_openpower_dump_dirs.service']
   ]

# Configuration header file(opconfig.h) generation
conf_data = configuration_data()

conf_data.set_quoted('SYSTEM_DUMP_OBJPATH', get_option('SYSTEM_DUMP_OBJPATH'),
                      description : 'The system Dump manager Dbus path'
                    )
conf_data.set_quoted('SYSTEM_DUMP_OBJ_ENTRY', get_option('SYSTEM_DUMP_OBJ_ENTRY'),
                      description : 'The system dump entry DBus object path'
                    )

conf_data.set_quoted('RESOURCE_DUMP_OBJPATH', get_option('RESOURCE_DUMP_OBJPATH'),
                      description : 'The resource Dump manager Dbus path'
                    )
conf_data.set_quoted('RESOURCE_DUMP_OBJ_ENTRY', get_option('RESOURCE_DUMP_OBJ_ENTRY'),
                      description : 'The resource dump entry DBus object path'
                    )

conf_data.set_quoted('HOSTBOOT_DUMP_OBJPATH', get_option('HOSTBOOT_DUMP_OBJPATH'),
                      description : 'The Hostboot Dump manager Dbus path'
                    )
conf_data.set_quoted('HOSTBOOT_DUMP_TMP_FILE_DIR', get_option('HOSTBOOT_DUMP_TMP_FILE_DIR'),
                      description : 'Directory where hostboot dump pieces are stored for packaging'
                    )
conf_data.set_quoted('HOSTBOOT_DUMP_PATH', get_option('HOSTBOOT_DUMP_PATH'),
                     description : 'Directory where Hostboot dumps are placed'
             )
conf_data.set('HOSTBOOT_DUMP_MAX_SIZE', get_option('HOSTBOOT_DUMP_MAX_SIZE'),
               description : 'Maximum size of one Hostboot dump in kilo bytes'
             )
conf_data.set('HOSTBOOT_DUMP_MIN_SPACE_REQD', get_option('HOSTBOOT_DUMP_MIN_SPACE_REQD'),
               description : 'Minimum space required for one Hostboot dump in kilo bytes'
             )
conf_data.set('HOSTBOOT_DUMP_TOTAL_SIZE', get_option('HOSTBOOT_DUMP_TOTAL_SIZE'),
               description : 'Total size of the dump in kilo bytes'
             )

configure_file(configuration : conf_data,
               output : 'opconfig.h'
              )

phosphor_dump_manager_sources += [
        'dump-extensions/openpower-dumps/dump-extensions.cpp',
        'dump-extensions/openpower-dumps/dump_manager_system.cpp',
        'dump-extensions/openpower-dumps/system_dump_entry.cpp',
        'dump-extensions/openpower-dumps/dump_manager_resource.cpp',
        'dump-extensions/openpower-dumps/resource_dump_entry.cpp',
        'dump-extensions/openpower-dumps/dump_manager_hostboot.cpp',
    ]
