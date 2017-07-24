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
                              Default output directory is /tmp
        -i, —-id <id>         Dump identifier to associate with the archive.
                              Identifiers include numeric characters.
                              Default dump identifier is 0
        -t, —-type <type>     Data collection type. Valid types are
                              "user", "core".
                              Default type is "user" initiated.
        -f, —-file <file>     Optional file to be included in the archive.
                              Absolute path of the file must be passed as
                              paramter. This is useful to include application
                              core in the dump.
        -s, --size <size>     Maximum allowed size(in KB) of the archive.
                              Report will be truncated incase size exceeds
                              this limit. Default size is 500KB.
        -v, —-verbose         Increase logging verbosity.
        -V, --version         Output version information.
        -q, —-quiet           Only log fatal errors to stderr
        -h, —-help            help Display usage message.
"

while [[ $# -ge 1 ]]; do
   key="$1"
   case $key in
      -h|--help)
        echo "$help"
        exit;;
      *)
        echo "Unknown option $1. Display available options with -h or --help"
        exit;;
  esac
done
