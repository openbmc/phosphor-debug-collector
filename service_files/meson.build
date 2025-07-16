systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
    pkgconfig: 'systemd_system_unit_dir',
)

foreach svc : [
    'obmc-dump-monitor.service',
    'ramoops-monitor.service',
    'xyz.openbmc_project.Dump.Manager.service',
]
    install_data(svc, install_dir: systemd_system_unit_dir)
endforeach
