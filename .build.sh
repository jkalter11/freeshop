#!/bin/sh
set -ex

mkdir -p build

# Install libarchive for host
apt-get update
apt-get -y install libarchive-dev

# Install libarchive for 3ds
git clone https://github.com/Cruel/3ds_portlibs.git
cd 3ds_portlibs
make libarchive
make install

cd ../build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
