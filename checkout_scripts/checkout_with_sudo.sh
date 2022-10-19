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
if [ ! -d "pugixml-1.12.1" ]; then
    wget http://github.com/zeux/pugixml/releases/download/v1.12.1/pugixml-1.12.1.tar.gz
    tar -xf pugixml-1.12.1.tar.gz
    rm pugixml-1.12.1.tar.gz
fi
pushd pugixml-1.12.1
mkdir -p build
cd build
cmake ..
# cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 2) nlohmann json, docu: https://json.nlohmann.me/, git: https://github.com/nlohmann/json
echo "Installing C++ JSON library..."
if [ ! -d "json-3.11.2" ]; then
    wget https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.tar.gz
    tar -xf v3.11.2.tar.gz
    rm v3.11.2.tar.gz
fi
pushd json-3.11.2
mkdir -p build
cd build
cmake ..
# cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 3) googletest, docu & git: https://github.com/google/googletest
echo "Installing C++ code testing library googletest..."
if [ ! -d "googletest-release-1.12.1" ]; then
    wget https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
    tar -xf release-1.12.1.tar.gz
    rm release-1.12.1.tar.gz
fi
pushd googletest-release-1.12.1
mkdir -p build
cd build
cmake ..
# cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# 4) simgrid, docu: https://simgrid.org/doc/latest/, git: https://framagit.org/simgrid/simgrid
echo "Installing SimGrid..."
if [ ! -d "simgrid-v3.32" ]; then
    wget https://framagit.org/simgrid/simgrid/-/archive/v3.32/simgrid-v3.32.tar.gz
    tar -xf simgrid-v3.32.tar.gz
    rm simgrid-v3.32.tar.gz
fi
pushd simgrid-v3.32
mkdir -p build
cd build
cmake ..
# cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd

# installing WRENCH 2.0:
echo "Installing WRENCH..."
if [ ! -d "wrench-v2.1"]; then
    wget https://github.com/wrench-project/wrench/archive/refs/tags/v2.1.tar.gz
    tar -xf v2.1.tar.gz -C wrench-v2.1
    rm v2.1.tar.gz
fi
pushd wrench-v2.1
mkdir -p build
cd build
cmake ..
# cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
# make -j 6 examples; sudo make install examples # needed additionally, since not done by default
popd

# install the sgbatch simulator
echo "Installing the DistCacheSim simulator..."
pushd $this_dir/../
mkdir -p build
cd build
cmake ..
# cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j 6; sudo make install
popd
