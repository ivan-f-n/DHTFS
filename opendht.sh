#!/bin/sh
set -ex
git clone https://github.com/savoirfairelinux/opendht.git
cd opendht
mkdir build && cd build
cmake -DOPENDHT_PEER_DISCOVERY=OFF -DOPENDHT_PYTHON=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
make -j4
sudo make install
