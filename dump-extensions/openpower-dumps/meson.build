# SPDX-License-Identifier: Apache-2.0

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
configure_file(configuration : conf_data,
               output : 'opconfig.h'
              )

phosphor_dump_manager_sources += [
        'dump-extensions/openpower-dumps/dump-extensions.cpp',
        'dump-extensions/openpower-dumps/dump_manager_system.cpp',
        'dump-extensions/openpower-dumps/system_dump_entry.cpp',
        'dump-extensions/openpower-dumps/dump_manager_resource.cpp',
        'dump-extensions/openpower-dumps/resource_dump_entry.cpp'
    ]
