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

## To Run
Multiple instances of `phosphor-dump-manager` are usually run on the bmc
to support management of different types of dumps.
```
Usage: ./phosphor-dump-manager [options]
Options:
    --help            Print this menu
    --type            Dump type
                      Valid types: bmc, system
    --host            Host identifies for host dump types
```

## D-Bus Interface
`phosphor-dump-manager` is an implementation of the D-Bus interface
defined in [this document]()

D-Bus service name is constructed by
"xyz.openbmc_project.Dump.Manager"
and D-Bus object path is constructed by
"/xyz/openbmc_project/Dump/{type}{host}".

