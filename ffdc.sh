#!/bin/bash

# Commands to be outputted into individual files
# Add more entries here as desired!
# Format: "File name" "Command"
declare -a arr=(
      "Build_info.txt"            "cat /etc/version"
      "FW_level.txt"              "cat /etc/os-release"
      "BMC_OS.txt"                "uname -a"
      "BMC_uptime.txt"            "uptime"
      "Disk_usage.txt"            "df -hT"

      "systemctl_status.txt"      "systemctl status | cat"
      "failed_services.txt"       "systemctl --failed"
      "obmc_console.txt"          "cat /var/log/obmc-console.log"

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
dest="/tmp"

while getopts ":d:" opt; do
  case $opt in
    d)
      if [ -d "$OPTARG" ]; then
        dest="$OPTARG" >&2
      else
        echo "Invalid destination specified: $OPTARG" >&2
      fi
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      ;;
  esac
done

echo "Using destination directory $dest ..."

mkdir "$dest/$dir"

for ((i=0;i<${#arr[@]};i+=2)); do
  echo "Collecting ${arr[i]}..."
  ${arr[i+1]} >> "$dest/$dir/${arr[i]}"
done

tar -cf "$dir.tar" -C $dest $dir
echo "Contents in $dest/$dir.tar"

rm -r "$dest/$dir"
