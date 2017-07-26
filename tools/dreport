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
declare -r SUMMARY_LOG="summary.log"
declare -r DREPORT_LOG="dreport.log"
declare -r TMP_DIR="/tmp/dreport"

#VARIABLES
declare -x name=$"obmcdump_00000000_$(date +"%s")"
declare -x dump_dir="/tmp"
declare -x dump_id=0
declare -x dump_type=$USERINITIATED_TYPE
declare -x verbose=$FALSE
declare -x quiet=$FALSE
declare -x dump_size=$DUMP_MAX_SIZE
declare -x name_dir="$TMP_DIR/$name"

# PACKAGE VERSION
PACKAGE_VERSION="0.0.1"

# @brief Packaging the dump and transferring to dump location.
function package()
{
    mkdir -p "$dump_dir"
    if [ $? -ne 0 ]; then
        log_error "Could not create the destination directory $dump_dir"
        dest_dir=$TMP_DIR
    fi

    #tar the files.
    tar_file="$name_dir.tar"
    tar -cf "$tar_file" -C "$TMP_DIR" "$name"

    #compress the tar file
    xz -z "$tar_file"

    #remove the temporary name specific directory
    rm -r "$name_dir"

    #check the file size is in the allowed limit
    file_size=$(stat -c%s "$tar_file.xz")

    if [ $(stat -c%s "$tar_file.xz") -gt $dump_size ]; then
       echo "File size exceeds the limit allowed"
       dump_dir="/tmp"
       #TODO openbmc/openbmc#1506 Revisit the error handling
    fi

    echo "Report is available in $dump_dir"

    if [ "$TMP_DIR" == "$dump_dir" ]; then
       return
    fi

    #copy the compressed tar file into the destination
    cp "$tar_file.xz" "$dump_dir"
    if [ $? -ne 0 ]; then
        echo "Failed to copy the $tar_file.xz to $dump_dir"
        return
    else
        rm -rf "$TMP_DIR"
    fi
}
# @brief log the error message
# @param error message
function log_error()
{
   if ((quiet == TRUE)); then
        echo "ERROR: $@" >> "$name_dir/$DREPORT_LOG"
   else
        echo "ERROR: $@" | tee -a "$name_dir/$DREPORT_LOG"
   fi
}

# @brief log warning message
# @param warning message
function log_warning()
{
    echo "Warning"
    if ((verbose == TRUE)); then
        if ((quiet == TRUE)); then
            echo "WARNING: $@" >> "$name_dir/$DREPORT_LOG"
        else
            echo "WARNING: $@" | tee -a "$name_dir/$DREPORT_LOG"
        fi
    fi
}

# @brief log info message
# @param info message
function log_info()
{
    if ((verbose == TRUE)); then
        if ((quiet == TRUE)); then
            echo "INFO: $@" >> "$name_dir/$DREPORT_LOG"
        else
            echo "INFO: $@" | tee -a "$name_dir/$DREPORT_LOG"
        fi
    fi
}

# @brief log summary message
# @param message
function log_summary()
{
   if ((quiet == TRUE)); then
        echo "$@" >> "$name_dir/$SUMMARY_LOG"
   else
        echo "$@" | tee -a "$name_dir/$SUMMARY_LOG"
   fi
}

# @brief Main function
function main()
{
   mkdir -p "$TMP_DIR/$name"
   if [ $? -ne 0 ]; then
      log_error "Failed to create the temporary directory."
      exit;
   fi

   log_summary "Version: $PACKAGE_VERSION"

   #TODO Add Dump report generating script.

   package  #package the dump
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
            dump_dir=$2
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
            log_error "Unknown argument: $1"
            log_info "$help"
            exit 1;;
    esac
done

main #main program
exit $?
