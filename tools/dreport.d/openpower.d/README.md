# opdreport

opdreport provides mechanisms to collect various log files and system parameters
of various platform dumps. Used to troubleshoot problems in OpenPower based
systems.

## To Extract

To extract the dump file, do the following steps:

```bash
    1. dd if=<dump file> of=<output file> bs=1 skip=1232
    2. tar -xvf <output file>
```
