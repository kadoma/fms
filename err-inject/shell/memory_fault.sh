#!/bin/sh

cpu=3
bank=2
channel=2
addr=0xab2000

max_num=10
i=0

# insmod mce
modprobe mce-inject

mempage_info=$(kfmadm -L -m $addr)
#echo $mempage_info
echo $mempage_info | grep online >/dev/null
page=$?

while [ $i -lt $max_num ]
do
   error=$(($(echo $RANDOM) % 8))
   status=$(printf "0x%08x%08x" $[0x94000000] $[0x80 + ($error << 4) + $channel])
   mce=$(printf "CPU %d BANK %d STATUS 0x%llx ADDR 0x%llx" $cpu $bank $status $addr)

#   echo $mce
   echo "mce-inject memory error"
if [ $page -eq 0 ] ; then
   echo $mce | mce-inject
fi
   sleep 1
   let i++
done

sleep 3

mempage_info=$(kfmadm -L -m $addr)
#echo $mempage_info
echo $mempage_info | grep online >/dev/null
page=$?

# fault, kernel panic
error_f=$(($(echo $RANDOM) % 8))
status_f=$(printf "0x%08x%08x" $[0xb4000000] $[0x80 + ($error << 4) + $channel])
mce_f=$(printf "CPU %d BANK %d STATUS 0x%llx ADDR 0x%llx" $cpu $bank $status_f $addr)

#echo $mce_f
echo "mce-inject memory fault"
if [ $page -eq 0 ] ; then
   echo $mce_f | mce-inject
fi
