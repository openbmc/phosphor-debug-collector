# phosphor-debug-collector
Phosphor Debug Collector provides mechanisms to collect various log files and
system parameters. Used to troubleshoot problems in OpenBMC based systems.

## To Build
To build this package with meson, do the following steps:
```
    1. meson builddir
    2. ninja -C builddir
```
To clean the built files run `ninja -C builddir clean`.

## To run unit tests
Tests can be run in the CI docker container, or with an OpenBMC x86 sdk(see
below for x86 steps).
```
meson -Doe-sdk=enabled -Dtests=enabled build
ninja -C build test
```
