#! /usr/bin/python3

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import os.path
import glob
import json

plt.rcParams["figure.figsize"] = [4., 3.]
plt.rcParams["figure.autolayout"] = True

njobs = 60
scenario = "copy"

scenario_plotlabel_dict = {
    "copy": "Input-files copied",
    "simplifiedstream": "Input-files streamed (simpl.)",
    "fullstream": "Block-streaming"
}

# create a dict of hitrate and corresponding simulation-trace JSON-output-files
outputfiles = glob.glob(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "sgbatch", "tmp", "outputs", f"unified*{njobs}jobs.json")))
hitrates = [float(outfile.split("_")[1].strip("h")) for outfile in outputfiles]
outputfiles_dict = dict(zip(hitrates,outputfiles))

# create a dataframe for each JSON file and add hitrate information
dfs = []
for hitrate, outputfile in outputfiles_dict.items():
    with open(outputfile) as f:
        df_tmp = pd.json_normalize(json.load(f)['workflow_execution']['tasks'])
        df_tmp['hitrate'] = hitrate
        dfs.append(df_tmp)


# concatenate all dataframes
if (all(os.path.exists(f) for f in outputfiles)):
    print("Found {} output-files!".format(len(outputfiles)))
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
ax1.set_title("Input-files copied")

ax1.set_xlabel('hitrate', loc='right')
ax1.set_ylabel('jobtime / min', color='black')
ax1.set_xlim([-0.05,1.05])
# ax1.set_ylim([0,400])

ax1.scatter(df['hitrate'], (df['whole_task.end']-df['whole_task.start'])/60., color='black', marker='x')
# ax1.grid(axis="y", linestyle = 'dotted', which='major')

h1, l1 = ax1.get_legend_handles_labels()
ax1.legend(h1, l1, loc=2)

fig.savefig(f"hitratescaling_{njobs}jobs.pdf")
