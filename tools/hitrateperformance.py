#! /usr/bin/python3

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import os.path
import argparse


plt.rcParams["figure.figsize"] = [4., 3.]
plt.rcParams["figure.autolayout"] = True


scenario_plotlabel_dict = {
    "copy": "Input-files copied",
    "fullstream": "Block-streaming",
    "SGBatch_fullstream_10G": "SG-Batch 10G gateway",
    "SGBatch_fullstream_1G": "SG-Batch 1G gateway",
    "SGBatch_fullstream_10G_50Mcache": "SG-Batch 10G gateway 50M cache"
}


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
    choices=scenario_plotlabel_dict.keys(),
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


machines = ['sg01', 'sg03', 'sg04']


# create a dict of hitrate and corresponding simulation-trace JSON-output-files
outputfiles = args.simoutputs
for outputfile in outputfiles:
    outputfile = os.path.abspath(outputfile)
    assert(os.path.exists(outputfile))

print("Found {0} output-files! Produce a hitrate scan for {0} hitrate values...".format(len(outputfiles)))


# create a dataframe for each CSV file and add hitrate information
dfs = []
for outputfile in outputfiles:
    with open(outputfile) as f:
        df_tmp = pd.read_csv(f, sep=",\s")
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


# plot the job-runtime dependence on hitrate
fig, ax1 = plt.subplots()
ax1.set_title(scenario_plotlabel_dict[scenario])

ax1.set_xlabel('hitrate', loc='right')
ax1.set_ylabel('jobtime / min', color='black')
ax1.set_xlim([-0.05,1.05])
# ax1.set_ylim([20,65])

# ax1 = df.plot.scatter(x='hitrate', y='walltime', c=)

scatter = ax1.scatter(
    df['hitrate'], ((df['job.end']-df['job.start'])/60), 
    c=df['machine.name'].astype('category').cat.codes, 
    marker='x'
)
# ax1.grid(axis="y", linestyle = 'dotted', which='major')

ax1.legend(
    handles=scatter.legend_elements()[0], 
    labels=machines,
    title="host"
)

fig.savefig(f"hitrateWalltime_{scenario}jobs{suffix}.pdf")
fig.savefig(f"hitrateWalltime_{scenario}jobs{suffix}.png")


fig2, ax2 = plt.subplots()
ax2.set_title(scenario_plotlabel_dict[scenario])

ax2.set_xlabel('hitrate', loc='right')
ax2.set_ylabel('transfer time / min', color='black')
ax2.set_xlim([-0.05,1.05])

scatter = ax2.scatter(
    df['hitrate'], ((df['infiles.transfertime']+df['outfiles.transfertime'])/60), 
    c=df['machine.name'].astype('category').cat.codes, 
    marker='x'
)

ax2.legend(
    handles=scatter.legend_elements()[0], 
    labels=machines,
    title="host"
)

fig2.savefig(f"hitrateIOtime_{scenario}jobs{suffix}.pdf")
fig2.savefig(f"hitrateIOtime_{scenario}jobs{suffix}.png")


fig3, ax3 = plt.subplots()
ax3.set_title(scenario_plotlabel_dict[scenario])

ax3.set_xlabel('hitrate', loc='right')
ax3.set_ylabel('CPU eff.', color='black')
ax3.set_xlim([-0.05,1.05])
ax3.set_ylim([0,1.05])

scatter = ax3.scatter(
    df['hitrate'], (df['job.computetime']/(df['job.end']-df['job.start'])), 
    c=df['machine.name'].astype('category').cat.codes, 
    marker='x'
)

ax3.legend(
    handles=scatter.legend_elements()[0], 
    labels=machines,
    title="host"
)

fig3.savefig(f"hitrateCPUeff_{scenario}jobs{suffix}.pdf")
fig3.savefig(f"hitrateCPUeff_{scenario}jobs{suffix}.png")