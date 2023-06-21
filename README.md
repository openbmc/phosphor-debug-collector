# phosphor-debug-collector

Phosphor Debug Collector provides mechanisms to collect various log files and
system parameters. Used to troubleshoot problems in OpenBMC based systems.

One such mechanism is [dreport](tools/dreport.d/README.md), a script that
collects debug data and packages it into an archive file.

## To Build

To build this package with meson, do the following steps:

```bash
    1. meson builddir
    2. ninja -C builddir
```

To clean the built files run `ninja -C builddir clean`.

## To run unit tests

Tests can be run in the CI docker container, refer
[local-ci-build](https://github.com/openbmc/docs/blob/master/testing/local-ci-build.md)

or with an OpenBMC x86 sdk(see below for x86 steps).

```bash
meson -Dtests=enabled build
ninja -C build test
```
