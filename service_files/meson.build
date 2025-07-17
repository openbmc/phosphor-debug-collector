systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
    pkgconfig: 'systemd_system_unit_dir',
)

install_data(
    [
        'obmc-dump-monitor.service',
        'ramoops-monitor.service',
        'xyz.openbmc_project.Dump.Manager.service',
    ],
    install_dir: systemd_system_unit_dir,
)
