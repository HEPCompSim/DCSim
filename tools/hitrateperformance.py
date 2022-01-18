#! /usr/bin/python3

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import os.path
import argparse


plt.rcParams["figure.figsize"] = [4., 3.]
plt.rcParams["figure.autolayout"] = True


def valid_file(param):
    base, ext = os.path.splitext(param)
    if ext.lower() not in ('.csv'):
        raise argparse.ArgumentTypeError('File must have a csv extension')
    return param


parser = argparse.ArgumentParser(
    description="Produce a plot containing the hitrate dependency of the simulated system. \
        It uses several files, one for each hitrate value to be represented in the scan. \
        The files containing the simulation dump are CSV files produced by the output method of the simulator.",
    add_help=True
)
parser.add_argument(
    "--scenario", 
    type=str,
    choices=("copy", "simplifiedstream", "fullstream"),
    required=True,
    help="Choose a scenario, which is used in the according plotting label and file-name of the plot."
)
parser.add_argument(
    "--suffix",
    type=str,
    help="Optonal suffix to add to the file-name of the plot."
)
parser.add_argument(
    "simoutputs",
    nargs='+',
    type=valid_file,
    help="CSV files containing information about the simulated jobs \
        produced by the simulator."
)


args = parser.parse_args()

scenario = args.scenario
suffix=args.suffix


scenario_plotlabel_dict = {
    "copy": "Input-files copied",
    "simplifiedstream": "Input-files streamed (simpl.)",
    "fullstream": "Block-streaming"
}


# create a dict of hitrate and corresponding simulation-trace JSON-output-files
outputfiles = args.simoutputs
for outputfile in outputfiles:
    outputfile = os.path.abspath(outputfile)
    assert(os.path.exists(outputfile))

print("Found {0} output-files! Produce a hitrate scan for {0} hitrate values...".format(len(outputfiles)))
hitrates = [float(outfile.split("_")[-1].strip(".csv").strip("hitrate")) for outfile in outputfiles]

outputfiles_dict = dict(zip(hitrates,outputfiles))
print(outputfiles_dict)


# create a dataframe for each JSON file and add hitrate information
dfs = []
for hitrate, outputfile in outputfiles_dict.items():
    with open(outputfile) as f:
        df_tmp = pd.read_csv(f, sep=',\t')
        df_tmp['hitrate'] = hitrate
        dfs.append(df_tmp)


# concatenate all dataframes
if (all(os.path.exists(f) for f in outputfiles)):
    df = pd.concat(
        [
            df
            for df in dfs
        ],
        ignore_index=True
    )
    print("Simulation task output traces: \n", df)
else:
    print("Couldn't find any files")
    exit(1)

# plot the job runtime dependence on hitrate
fig, ax1 = plt.subplots()
ax1.set_title(scenario_plotlabel_dict[scenario])

ax1.set_xlabel('hitrate', loc='right')
ax1.set_ylabel('jobtime / min', color='black')
ax1.set_xlim([-0.05,1.05])
# ax1.set_ylim([0,400])

ax1.scatter(df['hitrate'], (df['job.end']-df['job.start'])/60., color='black', marker='x')
# ax1.grid(axis="y", linestyle = 'dotted', which='major')

h1, l1 = ax1.get_legend_handles_labels()
ax1.legend(h1, l1, loc=2)

fig.savefig(f"hitratescaling_{scenario}jobs{suffix}.pdf")


fig2, ax2 = plt.subplots()
ax2.set_title(scenario_plotlabel_dict[scenario])

ax2.set_xlabel('hitrate', loc='right')
ax2.set_ylabel('transfer time / min', color='black')
ax2.set_xlim([-0.05,1.05])

ax2.scatter(df['hitrate'], ((df['infiles.transfertime']+df['outfiles.transfertime']))/60., color='black', marker='x')

h2, l2 = ax2.get_legend_handles_labels()
ax1.legend(h1, l1, loc=2)

fig2.savefig(f"hitratetransfer_{scenario}jobs{suffix}.pdf")