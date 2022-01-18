#! /usr/bin/python3

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import os.path
import glob
import argparse


plt.rcParams["figure.figsize"] = [4., 3.]
plt.rcParams["figure.autolayout"] = True


scenario_plotlabel_dict = {
    'withdump': "with JSON dump",
    'nodump': "without JSON dump",
    'private': "private improvements",
    "test": "final hacky-WRENCH"
}


def converttime(df: pd.DataFrame, a: str, b: str):
    return df[a].astype(int)*60 + df[b].astype(int)


parser = argparse.ArgumentParser(
    description="Produce a plot showing the runtime and memory scaling of the simulation. \
        If you intend to use this script, make sure that the monitoring files containing the \
        information about the simulation are in the right format. If you produced these by the \
        `simscaling.sh` script, it should work natively.",
    add_help=True
)
parser.add_argument(
    "--scenario", 
    type=str,
    choices=("withdump", "nodump", "private", "test"),
    required=True,
    help="Choose a scenario, which sets the according plotting label and filename of the plot."
)
parser.add_argument(
    "monitorfiles",
    nargs='+',
    help="Files containing monitoring information about the simulation run, produced by `ps -aux`.\
        Each file produces a single point in the plot for memory and runtime respectively."
)


args = parser.parse_args()

scenario = args.scenario


# Create a data-frame holding all monitoring information
monitorfiles = args.monitorfiles
print("Found {} monitorfiles".format(str(len(monitorfiles))))

if (all(os.path.exists(f) for f in monitorfiles) and monitorfiles):
    df = pd.concat(
        [
            pd.read_table(
                f,
                delimiter="\s+",
                names=[
                    "USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND",
                    "platform option", "Platform file", "njobs option", "NJobs", "ninfiles option", "NFilesPerJob",
                    "insize option", "FileSize", "hitrate option", "Hitrate", "output option", "OutputName",
                    "blockstreaming option"
                    ],
                )
            for f in monitorfiles
        ],
        ignore_index=True
        )
    print("Simulation monitoring information: \n", df)
else:
    print("Couldn't find any files")
    exit(1)


# postprocess data frame
df['TIME'] = df['TIME'].str.split(":",expand=True).pipe(converttime, 0, 1)
df['RSS'] = df['RSS']/(1024*1024)
runtimesdf = df.loc[df.groupby("NJobs")["TIME"].idxmax()]
memorydf = df.loc[df.groupby("NJobs")["RSS"].idxmax()]
print("Filtered data:\n", runtimesdf)


# Visualize the monitoring information
fig, ax = plt.subplots()
ax.set_title("Simulation scaling " + scenario_plotlabel_dict[scenario])

# ax.set_xscale('log')
ax.set_xlabel('$N_{jobs}$', loc='right')
ax.set_ylabel('time / min', color='black')
ax.set_xlim([0,2100])
# ax.set_ylim([0,400])

ax.plot(runtimesdf['NJobs'], runtimesdf['TIME']/60, linestyle='dotted', color='black')
ax.scatter(runtimesdf['NJobs'], runtimesdf['TIME']/60, color='black', marker='x', label='runtime')
ax.grid(axis="y", linestyle = 'dotted', which='major')

secax = ax.twinx()
secax.plot(memorydf['NJobs'], memorydf['RSS'],linestyle='dotted', color='orange')
secax.scatter(memorydf['NJobs'], memorydf['RSS'], color='orange', marker='^', label='memory')
# secax.xaxis.set_minor_locator(AutoMinorLocator())
secax.set_ylabel('memory / GiB', color='orange')
# secax.set_ylim([0,12])

h1, l1 = ax.get_legend_handles_labels()
h2, l2 = secax.get_legend_handles_labels()
ax.legend(h1+h2, l1+l2, loc=2)

fig.savefig("scalingtest_"+ scenario +".pdf")

# plt.show()