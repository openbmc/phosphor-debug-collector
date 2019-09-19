# phosphor-debug-collector
Phosphor Debug Collector provides mechanisms to collect various log files and
system parameters. Used to troubleshoot problems in OpenBMC based systems.

## Table Of Contents
* [Building](#to-build)
* [Dump collection](#dump-collection)
* [Dump Types](#dump-types)
* [REST commands](#rest-commands)

## To Build
```
To build this package, do the following steps:

    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS}
    3. make

To clean the repository run `./bootstrap.sh clean`.
```

## Dump collection
Based on the type of the dump, different plugins are run on the bmc to extract 
the journal traces, inventory, system state, core file, disk usage and package
it as zip file. See [here] [plugins] for plugins.

Total size of the dumps collected cannot exceed 1024 bytes. Before creating a
dump entry application checks if the new dump exceeds the maximum size.

Maximum size a particular dump can take is set to 200 bytes and minumum size is
set to 20 bytes.

Dumps are a collection of D-Bus interfaces that reside at
`/xyz/openbmc_project/dump/entry/1`, where X starts at 1 and is incremented for
each new dump.

Dumps are placed at `/var/lib/phosphor-debug-collector/dumps/`

During boot-up entries for the existing dumps are created.

## Dump types
1. core
2. user
3. elog
4. checkstop

### core dump
1. Dump triggered due to application core.
2. Dump manager watches for any new files created in 
"/var/lib/systemd/coredump" folder initiates a dump when a new core is
generated.

### user dump
1. Dump triggered by the user
```
busctl call xyz.openbmc_project.Dump.Manager \
/xyz/openbmc_project/dump xyz.openbmc_project.Dump.Create CreateDump
```
### elog dump
1. Dump triggered when the specified error is committed. List of errors to
watch for is captured [here] captured below list of errors for which dump is
triggered.

### checkstop dump
1. Dump triggered due to Checkstop type error commit

## REST commands

### Logging in
 * Before you can do anything, you need to first login.
```
$export bmc=xx.xx.xx.xx
$curl -c cjar -b cjar -k -X POST -H "Content-Type: application/json" -d \
'{"data": [ "root", "<root password>" ] }' https://{$bmc}/login
```

### List dump child objects recursively
```
$curl -c cjar -b cjar -k https://${bmc}/xyz/openbmc_project/dump/entry/list
```

### List dump attributes of child objects recursively
```
$curl -c cjar -b cjar -s -k -H 'Content-Type: application/json'; -d \
'{"data" : []}' -X GET https://${bmc}/xyz/openbmc_project/dump/enumerate
```

### Delete dump entries
```
$curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST -d \
'{"data": []}' \
https://${bmc}/xyz/openbmc_project/dump/entry/<entry num>/action/Delete
```

### Delete all dump entries
```
$curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST \
https://${bmc}/xyz/openbmc_project/dump/action/DeleteAll -d "{\"data\": [] }"
```

### user initiated dump
```
busctl call xyz.openbmc_project.Dump.Manager \
/xyz/openbmc_project/dump xyz.openbmc_project.Dump.Create CreateDump
```

## Links
[plugins]: https://github.com/openbmc/phosphor-debug-collector/tree/master/tools/dreport.d/plugins.d
[errors-watch]: https://github.com/openbmc/meta-phosphor/blob/master/recipes-phosphor/dump/phosphor-debug-errors/errors_watch.yaml

