#!/bin/sh

cpu=2
bank=2
level=1
type=1

max_num=7
i=0

# insmod mce
modprobe mce-inject

while [ $i -lt $max_num ]
do
   error=$(($(echo $RANDOM) % 9))
   status=$(printf "0x%08x%08x" $[0x90000000] $[0x100 + ($error << 4) + ($type << 2) + $level])
   mce=$(printf "CPU %d BANK %d STATUS 0x%llx" $cpu $bank $status)

#   echo $mce
   echo "mce-inject cpu error"  
   echo $mce | mce-inject  1>/dev/null 2>/dev/null

   sleep 1
   let i++
done


sleep 3

# fault, kernel panic
error_f=$(($(echo $RANDOM) % 9))
status_f=$(printf "0x%08x%08x" $[0xb0000000] $[0x100 + ($error << 4) + ($type << 2) + $level])
mce_f=$(printf "CPU %d BANK %d STATUS 0x%llx" $cpu $bank $status_f)

#echo $mce_f
echo "mce-inject cpu fault"  
echo $mce_f | mce-inject  1>/dev/null 2>/dev/null
