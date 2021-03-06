#!/bin/sh
set -ex
wget https://github.com/libfuse/libfuse/releases/download/fuse-3.10.3/fuse-3.10.3.tar.xz
tar -xf fuse-3.10.3.tar.xz
cd fuse-3.10.3/
sed -i '/^udev/,$ s/^/#/' util/meson.build &&
mkdir build &&
cd    build &&
meson --prefix=/usr .. &&
ninja
sudo ninja install
cd 
git clone https://github.com/savoirfairelinux/opendht.git
cd opendht
mkdir build && cd build
cmake -DOPENDHT_PEER_DISCOVERY=OFF -DOPENDHT_PYTHON=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
make -j4
sudo make install
