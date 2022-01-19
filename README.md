# DistCacheSim

Simulator for the simulation of high energy physics workflows on distributed computing systems with caching.

## Install instructions
To install `git clone` this repository and execute the checkout script inside:
```bash
source checkout.sh
```
Mind that you will need super-user rights to do so, as well as `cmake`, `git`, `clang` and `boost` installed on your system.
This will install the executable `sgbatch-sim` for this simulator and all its software dependencies.

## Usage
When you have successfully installed the simulator you can run
```bash
sgbatch-sim --help
```
to see possible options. 

Obligatory parameters are a platform file and a path and name for the resulting simulation output CSV-file:
```bash
sgbatch-sim -p <platform-file> -o <output-path>
```
The platform file has to follow the [SimGrid-defined DTD](https://simgrid.org/doc/latest/Platform.html).
The output-path can be any relative or absolute path of your file-system where you are allowed to write to.
