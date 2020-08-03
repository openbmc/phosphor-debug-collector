# phosphor-debug-collector
Phosphor Debug Collector provides mechanisms to collect various log files and
system parameters. Used to troubleshoot problems in OpenBMC based systems.

## To Build
```
To build this package, do the following steps:

    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS}
    3. make

To clean the repository run `./bootstrap.sh clean`.
```

## To Build with meson
To build this package with meson, do the following steps:
```
    1. meson builddir
    2. ninja -C builddir
```
To clean the built files run `ninja -C builddir clean`.

