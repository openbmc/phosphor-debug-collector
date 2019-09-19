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
the journal traces, inventory, system state, core file, disk usage, e.t.c and package
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

### Core dump
Dump triggered due to application core.
Dump manager watches for any new files created in
"/var/lib/systemd/coredump" folder initiates a dump when a new core is generated.

### User dump
Dump triggered by the user

### Elog dump
Dump triggered when the specified error is committed. List of errors to
watch for is captured [here] captured below list of errors for which dump is
triggered.

### Checkstop dump
Dump triggered due to Checkstop type error commit

## REST commands

### Logging in
 * Before you can do anything, generate token and export it as bmc_token
```
$export bmc=xx.xx.xx.xx
curl -k -H "Content-Type: application/json" -X POST https://${BMC_IP}/login -d '{"username" :  "rrr", "password" :  "XXXX"}'

```
### Creating user dump
```
curl -c cjar -b cjar -k -H "X-Auth-Token: $bmc_token" -d "{\"data\": []}" -X POST  https://$BMC_IP/xyz/openbmc_project/dump/action/CreateDump
```

### Listing dumps
```
curl -b cjar -k -H "X-Auth-Token: $bmc_token" https://${bmc}/xyz/openbmc_project/dump/entry/list
```

### List dump attributes of child objects recursively
```
curl -b cjar -s -k -H 'Content-Type: application/json' -H "X-Auth-Token: $bmc_token"  -d '{"data" : []}' -X GET  https://${bmc}/xyz/openbmc_project/dump/enumerate
```

### Delete dump entries
```
curl -b cjar -k -H "X-Auth-Token: $bmc_token" -d '{"data": []}'  -X POST  https://${bmc}/xyz/openbmc_project/dump/entry/1/action/Delete
```

### Delete all dump entries
```
curl -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/xyz/openbmc_project/dump/action/DeleteAll -d "{\"data\": [] }"
```

## Dump plugin
```
#!/bin/bash
#
# config: 23 5
desc="top"
file_name="top.log"
command="top -n 1 -b"

add_cmd_output "$command" "$file_name" "$desc"
```
A dump plugin will have config parameter that has two values for supported dump types and priority.
Supported dump types value specifies the dump types for which
the plugin is executed. For example: above top plugin script is
executed for dump type 2 (core) and 3 (elog).

[plugins]: https://github.com/openbmc/phosphor-debug-collector/tree/master/tools/dreport.d/plugins.d
[errors-watch]: https://github.com/openbmc/meta-phosphor/blob/master/recipes-phosphor/dump/phosphor-debug-errors/errors_watch.yaml

