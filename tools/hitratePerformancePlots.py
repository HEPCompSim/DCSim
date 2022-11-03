#! /usr/bin/python3

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import seaborn as sns
import os.path
import argparse


plt.rcParams['figure.autolayout'] = True
pd.set_option('display.max_columns',None)
plt.rcParams['axes.facecolor'] = 'white'
plt.rcParams['axes.spines.left'] = False
plt.rcParams['axes.spines.right'] = False
plt.rcParams['axes.spines.top'] = False
plt.rcParams['axes.spines.bottom'] = False
plt.rcParams['axes.grid'] = False
plt.rcParams['axes.grid.axis'] = 'both'
plt.rcParams['axes.labelcolor'] = '#555555'
plt.rcParams['text.color'] = 'black'
plt.rcParams['figure.figsize'] = 6,4
plt.rcParams['figure.dpi'] = 100
plt.rcParams['figure.titleweight'] = 'normal'
plt.rcParams['font.family'] = 'sans-serif'


scenario_plotlabel_dict = {
    "copy": "Input-files copied",
    "fullstream": "Block-streaming",
    "SGBatch_fullstream_10G": "SG-Batch 10Gb/s gateway",
    "SGBatch_fullstream_1G": "SG-Batch 1Gb/s gateway",
    "SGBatch_fullstream_10G_70Mcache": "SG-Batch 10Gb/s gateway 70MB/s cache"
}


def valid_file(param):
    base, ext = os.path.splitext(param)
    if ext.lower() not in (".csv"):
        raise argparse.ArgumentTypeError("File must have a csv extension")
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


# Derive quantities
df["Walltime"] = (df["job.end"]-df["job.start"])/60
df["IOtime"] = (df["infiles.transfertime"]+df["outfiles.transfertime"])/60
df["Efficiency"] = df["job.computetime"]/(df["job.end"]-df["job.start"])

# plot and save

fig = plt.figure("hitrate-walltime", figsize=(6,4))

ax1 = sns.scatterplot(x="hitrate", y="Walltime", hue="machine.name", data=df, alpha=0.9)
ax1.set_title(scenario_plotlabel_dict[scenario])
ax1.set_xlabel("hitrate", loc="right")
ax1.set_ylabel("jobtime / min", color="black")
ax1.set_xlim([-0.05,1.05])
# ax1.set_ylim([20,65])
ax1.legend(loc='best')

fig.savefig(f"hitrateWalltime_{scenario}jobs{suffix}.pdf")
fig.savefig(f"hitrateWalltime_{scenario}jobs{suffix}.png")


fig2 = plt.figure("hitrate-transfertime", figsize=(6,4))

ax2 = sns.scatterplot(x="hitrate", y="IOtime", hue="machine.name", data=df, alpha=0.9)
ax2.set_title(scenario_plotlabel_dict[scenario])
ax2.set_xlabel("hitrate", loc="right")
ax2.set_ylabel("transfer time / min", color="black")
ax2.set_xlim([-0.05,1.05])
ax2.legend(loc='best')

fig2.savefig(f"hitrateIOtime_{scenario}jobs{suffix}.pdf")
fig2.savefig(f"hitrateIOtime_{scenario}jobs{suffix}.png")


fig3= plt.figure("hitrate-efficiency", figsize=(6,4))

ax3 = sns.scatterplot(x="hitrate", y="Efficiency", hue="machine.name", data=df, alpha=0.9)
ax3.set_title(scenario_plotlabel_dict[scenario])
ax3.set_xlabel("hitrate", loc="right")
ax3.set_ylabel("CPU eff.", color="black")
ax3.set_xlim([-0.05,1.05])
ax3.set_ylim([0,1.05])
ax3.legend(loc='best')

fig3.savefig(f"hitrateCPUeff_{scenario}jobs{suffix}.pdf")
fig3.savefig(f"hitrateCPUeff_{scenario}jobs{suffix}.png")