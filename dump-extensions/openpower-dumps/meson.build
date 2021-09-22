# SPDX-License-Identifier: Apache-2.0

# Configuration header file(openpower_dumps_config.h) generation
opconf_data = configuration_data()

opconf_data.set_quoted('SYSTEM_DUMP_OBJPATH', get_option('SYSTEM_DUMP_OBJPATH'),
                      description : 'The system dump manager D-Bus path'
                    )
opconf_data.set_quoted('SYSTEM_DUMP_OBJ_ENTRY', get_option('SYSTEM_DUMP_OBJ_ENTRY'),
                      description : 'The system dump entry D-Bus object path'
                    )

opconf_data.set_quoted('SYSTEM_DUMP_SERIAL_PATH', get_option('SYSTEM_DUMP_SERIAL_PATH'),
                      description : 'The system dump D-Bus object persist path'
                    )

opconf_data.set_quoted('RESOURCE_DUMP_OBJPATH', get_option('RESOURCE_DUMP_OBJPATH'),
                      description : 'The resource dump manager D-Bus path'
                    )
opconf_data.set_quoted('RESOURCE_DUMP_OBJ_ENTRY', get_option('RESOURCE_DUMP_OBJ_ENTRY'),
                      description : 'The resource dump entry D-Bus object path'
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
        'dump-extensions/openpower-dumps/system_dump_serialize.cpp',
    ]
