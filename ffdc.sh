#!/bin/bash

# Commands to be outputted into individual files
# Add more entries here as desired!
# Format: "File name" "Command"
declare -a arr=(
      "Build_info.txt"      "cat /etc/version"
      "FW_level.txt"        "cat /etc/os-release"
      "OS.txt"              "uname -a"
      "Uptime.txt"          "uptime"
      "Disk_usage.txt"      "df -hT"

      "systemctl_status"    "systemctl status | cat"

      "BMC_proc_list.txt"   "top -n 1 -b"
      "BMC_journalctl.txt"  "journalctl --no-pager"
      "BMC_dmesg.txt"       "dmesg"
      "BMC_procinfo.txt"    "cat /proc/cpuinfo"
      "BMC_meminfo.txt"     "cat /proc/meminfo"
)

dir=$"ffdc_$(date +"%Y-%m-%d_%H-%M-%S")"
mkdir "$dir"

echo -n "Please enter destination folder (/tmp if left blank or invalid): "
read dest
if [ ! -d "$dest" ]; then
  echo "Not a valid destination. Using /tmp instead..."
  dest="/tmp"
fi

for ((i=0;i<${#arr[@]};i+=2)); do
  echo "Collecting ${arr[i]}..."
  ${arr[i+1]} >> "$dir/${arr[i]}"
done

tar -cf "$dest/$dir.tar" "$dir"
echo "Contents in $dest/$dir.tar"

rm -r "$dir"
