#! /bin/bash
set -e

# script to setup software in development mode for wrench and our caching simulation project
# root privileges are required to make this script running
#
# before checking out the git repositories, please install following prerequisites by the package
# manager of your OS:
#
# - cmake: version 3.7 or higher (currently using 3.18.4)
# - g++: version 6.3 or higher (currently using 11.2.0), OR
# - clang: version 3.8 or higher (currently using 13.0.0-2)
# - boost: version v1.59 or higher (currently using 1.74.0)
#
# checking out packages from git as prerequisites for WRENCH:
#
# 1) pugixml, docu: https://pugixml.org/docs/manual.html, git: https://github.com/zeux/pugixml

git clone git@github.com:zeux/pugixml.git # master branch, currently on commmit: 9e382f98076e57581fcc61323728443374889646
pushd pugixml
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 2) nlohmann json, docu: https://json.nlohmann.me/, git: https://github.com/nlohmann/json

git clone git@github.com:nlohmann/json.git # develop branch, currently on commit: 293f67f9ff1a3c7d16cf0959c95761e89b9b64e9
pushd json
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 3) simgrid, docu: https://simgrid.org/doc/latest/, git: https://framagit.org/simgrid/simgrid

git clone https://framagit.org/simgrid/simgrid.git # master branch, currently on commit: f0c07d4ab3b94286d109ff88493b01c082ad70cb
pushd simgrid
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# installing WRENCH 2.0:

git clone --branch wrench-2.0 git@github.com:wrench-project/wrench.git # wrench-2.0 branch, currently on commit: ae27c2d22cc68b4077be0e1be97a26b3f2199a8d
pushd wrench
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
make -j 6 examples; sudo make install examples # needed additionally, since not done by default
popd

# installing repo for our caching simulation:

git clone --branch test/sgbatch-wrench-2.0 git@github.com:HerrHorizontal/DistCacheSim.git # test/sgbatch-wrench-2.0 branch
pushd DistCacheSim/sgbatch
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd
