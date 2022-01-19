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

wget http://github.com/zeux/pugixml/releases/download/v1.11/pugixml-1.11.tar.gz
tar -xf pugixml-1.11.tar.gz
pushd pugixml-1.11
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 2) nlohmann json, docu: https://json.nlohmann.me/, git: https://github.com/nlohmann/json

wget https://github.com/nlohmann/json/archive/refs/tags/v3.10.4.tar.gz
tar -xf v3.10.4.tar.gz
pushd json-3.10.4
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 3) googletest, docu & git: https://github.com/google/googletest
wget https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz
tar -xf release-1.11.0.tar.gz
pushd googletest-release-1.11.0
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 4) simgrid, docu: https://simgrid.org/doc/latest/, git: https://framagit.org/simgrid/simgrid

wget https://framagit.org/simgrid/simgrid/-/archive/v3.29/simgrid-v3.29.tar.gz
tar -xf simgrid-v3.29.tar.gz
pushd simgrid-v3.29
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# installing WRENCH-1.10 based hacky-WRENCH:

wget https://github.com/HerrHorizontal/wrench/archive/refs/tags/hacky-WRENCH.tar.gz
tar -xf hacky-WRENCH
pushd wrench-hacky-WRENCH
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
make -j 6 examples; sudo make install examples # needed additionally, since not done by default
popd
