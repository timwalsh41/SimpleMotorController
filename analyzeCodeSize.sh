#!/bin/bash

dir="Debug"

if [ $# -eq 1 ]; then
    dir="$1"
fi

declare -A total_text
declare -A total_data
declare -A total_bss

while read -r file; do
    subdir=$(dirname "$file")
    subdir=${subdir#"$dir/"}
    echo "$file"
    size_output=$(arm-none-eabi-size "$file")
    echo "$size_output"
    text=$(echo "$size_output" | tail -n 1 | awk '{print $1}')
    data=$(echo "$size_output" | tail -n 1 | awk '{print $2}')
    bss=$(echo "$size_output" | tail -n 1 | awk '{print $3}')
    total_text["$subdir"]=$((total_text["$subdir"] + text))
    total_data["$subdir"]=$((total_data["$subdir"] + data))
    total_bss["$subdir"]=$((total_bss["$subdir"] + bss))
    echo
done < <(find "$dir" -type f -name "*.o")

for subdir in "${!total_text[@]}"; do
    echo "Subfolder: $subdir"
    echo "Total text: ${total_text["$subdir"]}"
    echo "Total data: ${total_data["$subdir"]}"
    echo "Total bss: ${total_bss["$subdir"]}"
    echo
done

grand_total_text=0
grand_total_data=0
grand_total_bss=0

for subdir in "${!total_text[@]}"; do
    grand_total_text=$((grand_total_text + total_text["$subdir"]))
    grand_total_data=$((grand_total_data + total_data["$subdir"]))
    grand_total_bss=$((grand_total_bss + total_bss["$subdir"]))
done

echo "Overall totals:"
echo "Total text: $grand_total_text"
echo "Total data: $grand_total_data"
echo "Total bss: $grand_total_bss"
