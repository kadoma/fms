#!/bin/sh

cd ..
make uninstall
make clean
make
make install
mkdir /dev/mqueue
mount -t mqueue none /dev/mqueue
cp scripts/fmd.conf /etc/ld.so.conf.d
ldconfig
./cmd/fmd/fmd
