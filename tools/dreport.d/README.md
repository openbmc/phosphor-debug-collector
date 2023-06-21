# dreport

dreport is a shell script that uses plugin shell scripts to collect debug
information about the system and package it into a tar.xz archive file.

The phosphor-dump-manager application will automatically run dreport in certain
failure cases, and it can also be run manually by running `dreport` or
`dreport -v`.

## Dump Types

The dump types are defined in [sample.conf](sample.conf):

- core: Triggered by an application core dump
- user: The type when manually called, such as from running `dreport`.
- elog: Triggered when there are specific phosphor-logging event logs.
- checkstop: Triggered on a specific type of host crash.
- ramoops: Triggered when there is a kernel panic.

## Plugins

The plugins are the shell scripts in the [plugins.d](plugins.d) subdirectory.
They can call [provided functions](include.d/functions) to add data to the dump
archive. Each plugin needs a comment line like the following to specify which
dump types should trigger it:

```bash
# config A B
```

- 'A' is a sequence of the dump types from the mapping defined in
  [sample.conf](sample.conf).
- 'B' is a priority determines the order the plugins run in.

For example, the `bmcstate` plugin has:

```bash
# config: 12345 5
```

It will run on dump types 1 (core), 2 (user), 3 (checkstop), 4 (checkstop), and
5 (ramooops) with a priority of 5.

During the bitbake build, the script will be linked into a directory based on
the dump type, with the priority built into the name:

```bash
/usr/share/dreport.d# find /usr/share/dreport.d/ -name *bmcstate
/usr/share/dreport.d/plugins.d/bmcstate
/usr/share/dreport.d/pl_user.d/E5bmcstate
/usr/share/dreport.d/pl_checkstop.d/E5bmcstate
/usr/share/dreport.d/pl_elog.d/E5bmcstate
/usr/share/dreport.d/pl_core.d/E5bmcstate
```
