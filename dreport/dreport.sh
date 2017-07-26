#! /bin/bash

help=$"
        dreport creates an archive(xz compressed) consisting of the following:
                * Configuration information
                * Debug information
                * A summary report
        The type parameter controls the content of the data. The generated
        archive is stored in the user specified location.

usage: dreport [OPTION]

Options:
        -n, —-name <name>     Name to be used for the archive.
                              Default name format obmcdump_<id>_<epochtime>
        -d, —-dir <directory> Archive directory to copy the compressed report.
                              Default output directory is /tmp/dreport
        -i, —-id <id>         Dump identifier to associate with the archive.
                              Identifiers include numeric characters.
                              Default dump identifier is 0
        -t, —-type <type>     Data collection type. Valid types are
                              "user", "core".
                              Default type is "user" initiated.
        -f, —-file <file>     Optional file to be included in the archive.
                              Absolute path of the file must be passed as
                              parameter. This is useful to include application
                              core in the dump.
        -s, --size <size>     Maximum allowed size(in KB) of the archive.
                              Report will be truncated in case size exceeds
                              this limit. Default size is 500KB.
        -v, —-verbose         Increase logging verbosity.
        -V, --version         Output version information.
        -q, —-quiet           Only log fatal errors to stderr
        -h, —-help            Display this help and exit.
"

#CONSTANTS
declare -r TRUE=1
declare -r FALSE=0
declare -r USERINITIATED_TYPE=0
declare -r APPLICATIONCORED_TYPE=1
declare -r DUMP_MAX_SIZE=500 #in KB

#VARIABLES
declare -x name=$"obmcdump_00000000_$(date +"%s")"
declare -x dump_dir=$TMP_DIR
declare -x dump_id=1
declare -x dump_type=$USERINITIATED_TYPE
declare -x verbose=$FALSE
declare -x quiet=$FALSE
declare -x dump_size=$DUMP_MAX_SIZE

# PACKAGE VERSION
PACKAGE_VERSION="0.0.1"

# @brief Main function
function main()
{
   echo "Initial Version of dreport tool"
}

TEMP=`getopt -o n:d:i:t:s:f:vVqh \
      --long name:,dir:,dumpid:,type:,size:,file:,verbose,version,quiet,help \
      -- "$@"`
eval set -- "$TEMP"

while [[ $# -gt 1 ]]; do
    key="$1"
    case $key in
        -n|--name)
            name=$2
            shift 2;;
        -d|--dir)
            dir=$2
            shift 2;;
        -i|--dumpid)
            dump_id=$2
            shift 2;;
        -t|--type)
            dump_type=$2
            shift 2;;
        -s|--size)
            dump_size=$2
            shift 2;;
        -f|--file)
            dump_file=$2
            shift 2;;
        -v|—-verbose)
            verbose=$TRUE
            shift;;
        -V|--version)
            shift;;
        -q|—-quiet)
            quiet=$TRUE
            shift;;
        -h|--help)
            echo "$help"
            exit;;
        *) # unknown option
            echo "Unknown argument: $1"
            echo "$help"
            exit 1;;
    esac
done

main #main program
exit $?
