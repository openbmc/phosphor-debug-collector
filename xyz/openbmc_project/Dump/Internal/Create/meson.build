# SPDX-License-Identifier: Apache-2.0

sdbuscpp = find_program('sdbus++')

server_hpp = custom_target(
                'server.hpp',
                command : [
                    sdbuscpp, '-r', meson.source_root(),
                    'interface',
                    'server-header',
                    'xyz.openbmc_project.Dump.Internal.Create',
                ],
                input : '../Create.interface.yaml',
                capture : true,
                output : 'server.hpp',
                install : true,
                install_dir: get_option('includedir') / 'xyz/openbmc_project/Dump/Internal/Create'
            )

server_cpp = custom_target(
                'server.cpp',
                command : [
                    sdbuscpp, '-r', meson.source_root(),
                    'interface',
                    'server-cpp',
                    'xyz.openbmc_project.Dump.Internal.Create'
                ],
                input : '../Create.interface.yaml',
                depends : server_hpp,
                capture : true,
                output : 'server.cpp'
            )
