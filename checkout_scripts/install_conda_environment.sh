#! /usr/bin/bash
ulimit -s unlimited

NCORES=12

export HOME=$(pwd)
echo "INITIAL ENVIRONMENT START"
env
echo "INITIAL ENVIRONMENT END"
echo ""

if [ -x "$(command -v conda)" ]
then
    source $(pwd)/miniconda/bin/activate
else
    wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
    chmod u+x Miniconda3-latest-Linux-x86_64.sh
    ./Miniconda3-latest-Linux-x86_64.sh -b -p $(pwd)/miniconda
    source $(pwd)/miniconda/bin/activate
    conda config --set auto_activate_base false
    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda update -n base conda -y
    conda create -n dcsim-env cmake python=3.10 pip gcc gxx make gfortran boost git conda-pack -y
    conda activate dcsim-env
    python3 -m pip install pip setuptools numpy matplotlib scipy pandas --upgrade --no-input
    conda env config vars set LD_LIBRARY_PATH=${CONDA_PREFIX}/lib:${CONDA_PREFIX}/lib64:${CONDA_PREFIX}/lib32
    conda deactivate
fi

conda activate dcsim-env

echo "FINAL CONDA ENVIRONMENT START"
env
echo "FINAL CONDA ENVIRONMENT END"
echo ""

echo "CONDA PACKAGES:"
conda list
echo "PIP PACKAGES:"
python -m pip list
echo "PIP PACKAGES OUTDATED:"
python -m pip list --outdated

echo "INSTALLING SIMULATION SOFWARE PACKAGES"
mkdir -p CachingSimulation; cd CachingSimulation

# pugixml
git clone https://github.com/zeux/pugixml.git
mkdir -p pugixml/build
pushd pugixml/build
git checkout tags/v1.12.1
cmake -DCMAKE_INSTALL_PREFIX=${CONDA_PREFIX} ../
make -j${NCORES}
make install
popd

# json
git clone https://github.com/nlohmann/json.git
mkdir -p json/build
pushd json/build
git checkout tags/v3.11.2
cmake -DCMAKE_INSTALL_PREFIX=${CONDA_PREFIX} ../
make -j${NCORES}
make install
popd

# googletest
git clone https://github.com/google/googletest.git
mkdir -p googletest/build
pushd googletest/build
git checkout tags/release-1.12.1
cmake -DCMAKE_INSTALL_PREFIX=${CONDA_PREFIX} ../
make -j${NCORES}
make install
popd

# simgrid
git clone https://framagit.org/simgrid/simgrid.git
mkdir -p simgrid/build
pushd simgrid/build
git checkout tags/v3.31
cmake -DCMAKE_INSTALL_PREFIX=${CONDA_PREFIX} ../
make -j${NCORES}
make install
popd

# wrench
git clone https://github.com/wrench-project/wrench.git
mkdir -p wrench/build
pushd wrench/build
cmake -DCMAKE_INSTALL_PREFIX=${CONDA_PREFIX} ../
make -j${NCORES}
make install
popd

# DCSim
git clone https://github.com/HEPCompSim/DCSim.git
mkdir -p DCSim/build
pushd DCSim/build
git checkout extension/platform
cmake -DCMAKE_INSTALL_PREFIX=${CONDA_PREFIX} ../
make -j${NCORES}
make install
popd

echo "RUNNING TEST COMMAND:"
cd DCSim
/usr/bin/time -v sgbatch-sim -p data/platform-files/sgbatch_scaletest.xml -o test.csv -n 60
