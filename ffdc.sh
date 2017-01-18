#!/bin/bash

help=$'FFDC File Collection Script

Collects various FFDC files and system parameters and places them in a .tar

usage: sh ffdc.sh [OPTION]

Options:
   -d, --dir <directory>  Specify destination directory. Defaults to /tmp if
                          invalid or unspecified.
   -h, --help             Display this help text and exit.
'

declare -a arr=(
# Commands to be outputted into individual files
# Format: "File name" "Command"
      "Build_info.txt"            "cat /etc/version"
      "FW_level.txt"              "cat /etc/os-release"
      "BMC_OS.txt"                "uname -a"
      "BMC_uptime.txt"            "uptime"
      "BMC_disk_usage.txt"        "df -hT"

      "systemctl_status.txt"      "systemctl status | cat"
      "failed_services.txt"       "systemctl --failed"
      "host_console.txt"          "cat /var/log/obmc-console.log"

      "BMC_proc_list.txt"         "top -n 1 -b"
      "BMC_journalctl.txt"        "journalctl --no-pager"
      "BMC_dmesg.txt"             "dmesg"
      "BMC_procinfo.txt"          "cat /proc/cpuinfo"
      "BMC_meminfo.txt"           "cat /proc/meminfo"

# Copy all content from these directories into directories in the .tar
# Format: "Directory name" "Directory to copy"
      "obmc"                      "/var/lib/obmc/"
)

dir=$"ffdc_$(date +"%Y-%m-%d_%H-%M-%S")"
dest="/tmp"

while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    -d|--dir)
      if [ -d "$2" ]; then
        dest="$2"
      else
        echo "Invalid or no destination directory specified."
        break
      fi
      shift 2
      ;;
    -h|--help)
      echo "$help"
      exit
      ;;
    *)
      echo "Unknown option $1. Display available options with -h or --help"
      exit
      ;;
  esac
done

echo "Using destination directory $dest"

mkdir "$dest/$dir"

for ((i=0;i<${#arr[@]};i+=2)); do
  if [ -d "${arr[i+1]}" ]; then
    echo "Copying contents of ${arr[i+1]} to directory ./${arr[i]} ..."
    mkdir "$dest/$dir/${arr[i]}"
    cp -r "${arr[i+1]}/*" "$dest/$dir/${arr[i]}"
  else
    echo "Collecting ${arr[i]}..."
    ${arr[i+1]} >> "$dest/$dir/${arr[i]}"
  fi
done

tar -cf "$dir.tar" -C $dest $dir
echo "Contents in $dest/$dir.tar"

rm -r "$dest/$dir"
