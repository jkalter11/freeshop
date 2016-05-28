#!/bin/sh
set -ex

mkdir -p build

git clone https://github.com/Cruel/3ds_portlibs.git
cd 3ds_portlibs
make libarchive
make install

cd ../build
cmake ..
make -j4
