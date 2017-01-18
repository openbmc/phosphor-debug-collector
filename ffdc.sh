# Commands to be outputted into individual files
# Add more entries here as desired!
# Format: "File name" "Command"
declare -a arr=(
      "Build_info"      "cat /etc/version"
      "FW_level"        "cat /etc/os-release"
      "OS"              "uname -a"
      "Uptime"          "uptime"
      "Disk_usage"      "df -hT"

      "BMC_proc_list"   "top -n 1 -b"
      "BMC_journalctl"  "journalctl --no-pager"
      "BMC_dmesg"       "dmesg"
      "BMC_procinfo"    "cat /proc/cpuinfo"
      "BMC_meminfo"     "cat /proc/meminfo"
)

dir=$"ffdc_$(date +"%Y-%m-%d_%H-%M-%S")"
mkdir "$dir"

for ((i=0;i<${#arr[@]};i+=2)); do
  ${arr[i+1]} >> "$dir/${arr[i]}"
done

tar -cf "$dir.tar" "$dir"
echo "Contents in $dir.tar"

rm -r "$dir"
