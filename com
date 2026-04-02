Fix watch descriptor leak for resource and sys dumps

Issue:
Currently in the opdump path, if any dump directory is created during
dump creation, it adds a watch descriptor to monitor for file close
event in that directory. While this works for host dumps, resource
and system dumps which are never captured in bmc local file system,
which will cause a watch descriptor leak as there will never be a
file close event for these dumps as they exist on host.

Fix:
Skip adding a watch descriptor for resource and system dump paths

Verified:
```
System and Resource dump paths are generated without any watches.
```

