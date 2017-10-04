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

## Dump Manager REST Interfaces

####Create User initiated Dump
     curl -c cjar -b cjar -k -H "Content-Type: application/json" -d "{\"data\": []}" -X POST  https://$BMC_IP/xyz/openbmc_project/dump/action/CreateDump

####List all Dumps
     curl -c cjar -b cjar -k https://$BMC_IP/xyz/openbmc_project/dump/list

####Retrieve a Dump
     curl -O -J -c cjar -b cjar -k -X GET https://$BMC_IP/download/dump/$ID

####Delete Dump
     curl -c cjar -b cjar -k -H "Content-Type: application/json" -d "{\"data\": []}" -X POST  https://$BMC_IP/xyz/openbmc_project/dump/entry/<ID>/action/Delete

####Delete all Dumps
     curl -c cjar -b cjar -k -H "Content-Type: application/json" -d "{\"data\": []}" -X POST  https://$BMC_IP/xyz/openbmc_project/dump/action/DeleteAll

####Example

     curl -c cjar -b cjar -k -H "Content-Type: application/json" -d "{\data\": []}" -X POST  https://$BMC_IP/xyz/openbmc_project/dump/action/CreateDump
    {
       "data": 1,  — > ID
       "message": "200 OK",
       "status": "ok"
    }
