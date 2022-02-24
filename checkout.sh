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

this_file="$( [ ! -z "$ZSH_VERSION" ] && echo "${(%):-%x}" || echo "${BASH_SOURCE[0]}" )"
this_dir="$( cd "$( dirname "$this_file" )" && pwd )"


# checking out packages from git as prerequisites for WRENCH:
#
# 1) pugixml, docu: https://pugixml.org/docs/manual.html, git: https://github.com/zeux/pugixml
echo "Installing C++ XML processing library pugixml..."
wget http://github.com/zeux/pugixml/releases/download/v1.11/pugixml-1.11.tar.gz
tar -xf pugixml-1.11.tar.gz
rm pugixml-1.11.tar.gz
pushd pugixml-1.11
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 2) nlohmann json, docu: https://json.nlohmann.me/, git: https://github.com/nlohmann/json
echo "Installing C++ JSON library..."
wget https://github.com/nlohmann/json/archive/refs/tags/v3.10.4.tar.gz
tar -xf v3.10.4.tar.gz
rm v3.10.4.tar.gz
pushd json-3.10.4
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 3) googletest, docu & git: https://github.com/google/googletest
echo "Installing C++ code testing library googletest..."
wget https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz
tar -xf release-1.11.0.tar.gz
rm release-1.11.0.tar.gz
pushd googletest-release-1.11.0
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 4) simgrid, docu: https://simgrid.org/doc/latest/, git: https://framagit.org/simgrid/simgrid
echo "Installing SimGrid..."
wget https://framagit.org/simgrid/simgrid/-/archive/v3.30/simgrid-v3.30.tar.gz
tar -xf simgrid-v3.30.tar.gz
rm simgrid-v3.30.tar.gz
pushd simgrid-v3.30
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# installing WRENCH 2.0:
echo "Installing WRENCH..."
git clone --branch wrench-2.0 git@github.com:wrench-project/wrench.git
pushd wrench
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
# make -j 6 examples; sudo make install examples # needed additionally, since not done by default
popd

# install the sgbatch simulator
echo "Installing the DistCacheSim simulator..."
pushd $this_dir/sgbatch
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd
