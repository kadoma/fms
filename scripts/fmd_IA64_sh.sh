#!/bin/sh

cd ..
make uninstall
make clean
make
make install
./scripts/post.sh
service fmd start
