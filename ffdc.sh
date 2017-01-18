#!/bin/bash

help=$'FFDC File Collection Script

Collects various FFDC files and system parameters and places them in a .tar

usage: sh ffdc.sh [arguments]

Arguments:
   -d       Specify destination dir. Defaults to /tmp if invalid or unspecified
   -h       Displays help text

   --dir    Alias for -d
   --help   Alias for -h
'

# Commands to be outputted into individual files
# Add more entries here as desired!
# Format: "File name" "Command"
declare -a arr=(
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

      "host0-boot_flags.txt"      "cat /var/lib/obmc/host0-boot_flags"
      "host0-boot_policy.txt"     "cat /var/lib/obmc/host0-boot_policy"
      "host0-power_cap.txt"       "cat /var/lib/obmc/host0-power_cap"
      "host0-system_state.txt"    "cat /var/lib/obmc/host0-system_state"
      "host0-time_mode.txt"       "cat /var/lib/obmc/host0-time_mode"
      "host0-time_owner.txt"      "cat /var/lib/obmc/host0-time_owner"
      "last-system-state.txt"     "cat /var/lib/obmc/last-system-state"
      "saved_host_offset.txt"     "cat /var/lib/obmc/saved_host_offset"
      "saved_timeMode.txt"        "cat /var/lib/obmc/saved_timeMode"
      "saved_timeOwner.txt"       "cat /var/lib/obmc/saved_timeOwner"
)

dir=$"ffdc_$(date +"%Y-%m-%d_%H-%M-%S")"

while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    -d|--dir)
      dest="$2"
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

if [ ! -d "$dest" ]; then
  echo "Invalid or unspecified destination directory."
  dest="/tmp"
fi

echo "Using destination directory $dest"

mkdir "$dest/$dir"

for ((i=0;i<${#arr[@]};i+=2)); do
  echo "Collecting ${arr[i]}..."
  ${arr[i+1]} >> "$dest/$dir/${arr[i]}"
done

tar -cf "$dir.tar" -C $dest $dir
echo "Contents in $dest/$dir.tar"

rm -r "$dest/$dir"
