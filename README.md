# DistCacheSim

Simulator for the simulation of high energy physics workflows on distributed computing systems with caching.


## Install instructions

### Option 1
To get a fresh installation on your own system, either `git clone` this repository and execute the checkout script inside:
```bash
source checkout_scripts/checkout_with_sudo.sh
```
Mind that you will need super-user rights to do so, as well as `cmake`, `git`, `clang` and `boost` installed on your system.
This will install the executable `dc-sim` for this simulator and all its software dependencies.

### Option 2
Create a `conda` environment using the provided script
```bash
checkout_scripts/install_conda_environment.sh
```
This will automatically take care of all the dependencies needed and include them into the environment.
To activate the environment, execute
```bash
conda activate dcsim-env
```
and deactivate it accordingly with 
```
conda deactivate
```.


## Usage
When you have successfully installed the simulator or activated the conda environment you can run
```bash
dc-sim --help
```
to see all possible execution options. 

Mandatory parameters are a platform file and a path and name for the resulting simulation output CSV-file:
```bash
dc-sim -p <platform-file> -o <output-path>
```
The platform file has to follow the [SimGrid-defined DTD](https://simgrid.org/doc/latest/Platform.html).
Example files can be found in `data/platform-files`.
The output-path can be any relative or absolute path of your file-system where you are allowed to write to.
Instead of manually setting up all workflow parameters via command line options, 
there is also the option to provide a JSON file, which contains all necessary information about a workflow by adding the option:
```bash
--workflow-configurations <path_to_workflow_json>
```
The workflow should contain the full information as it would be set via the command line, e.g.:
```json
{
    "name":"stream_and_compute_workflow",
    "num_jobs": 60,
    "infiles_per_job":10,
    "average_flops":2164428000000,
    "sigma_flops":216442800000,
    "average_memory":2000000000,
    "sigma_memory":200000000,
    "average_infile_size":3600000000,
    "sigma_infile_size":360000000,
    "average_outfile_size":18000000000,
    "sigma_outfile_size":1800000000,
    "workflow_type":"streaming"
}
```
It is also possible to give a list of workflow configuration files, which enables to simulate the execution of multiple workflows.
Example configuration files covering different workflow-types are given in `data/workflow-configs`.
