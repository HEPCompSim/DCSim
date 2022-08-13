#! /usr/bin/bash
ulimit -s unlimited
set -e

export HOME=$(pwd)
echo "INITIAL ENVIRONMENT START"
env
echo "INITIAL ENVIRONMENT END"
echo ""

wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
chmod u+x Miniconda3-latest-Linux-x86_64.sh
./Miniconda3-latest-Linux-x86_64.sh -b -p $(pwd)/miniconda
source $(pwd)/miniconda/bin/activate
conda config --add channels conda-forge
conda config --set channel_priority strict
conda update conda -y
conda create -p $(pwd)/dcs_env cmake python=3.10 pip gcc gxx make gfortran boost git -y
conda activate $(pwd)/dcs_env
python3 -m pip install pip setuptools numpy matplotlib scipy pandas --upgrade --no-input
conda env config vars set DESTDIR=${CONDA_PREFIX}
conda env config vars set LD_LIBRARY_PATH=${CONDA_PREFIX}/lib:${CONDA_PREFIX}/usr/local/lib64:${CONDA_PREFIX}/usr/local/lib
conda env config vars set CPATH=${CONDA_PREFIX}/usr/local/include
conda env config vars set CMAKE_PREFIX_PATH=${CONDA_PREFIX}/usr/local
conda env config vars set PATH=${CONDA_PREFIX}/usr/local/bin:${PATH}
conda deactivate
conda activate $(pwd)/dcs_env

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

echo "EXTRACTING SOFWARE PACKAGE"
tar -zxvf DCSim.tar.gz

echo "INSTALLING SOFWARE PACKAGE"
for d in `find DCSim/ -maxdepth 3 -type d -iname "build"`
do
  pushd ${d}
  make install
  popd
done

echo "RUNNING TEST COMMAND:"
cd DCSim
/usr/bin/time -v sgbatch-sim -p data/platform-files/sgbatch_scaletest.xml -o test.csv -n 60
