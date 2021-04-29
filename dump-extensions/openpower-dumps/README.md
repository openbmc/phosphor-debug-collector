# openpower-dumps
openpower-dumps provides mechanisms to collect various log files and
system parameters of various platform dumps.

## To Extract
To extract the dump file, do the following steps:
```
    1.gunzip <dump file>
    2.dd if=<dump file> of=<output file> bs=1 skip=1232
    3.tar -xvf <output file>
```
