#! /usr/bin/python3

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import os.path
import glob
import datetime

plt.rcParams["figure.figsize"] = [4., 3.]
plt.rcParams["figure.autolayout"] = True

scenario = 'nodump'

scenario_plotlabel_dict = {
    'withdump': "with JSON dump",
    'nodump': "without JSON dump"
}


def converttime(df: pd.DataFrame, a: str, b: str):
    return df[a].astype(int)*60 + df[b].astype(int)


# Create a data-frame holding all monitoring information
monitorfiles = glob.glob(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "sgbatch", "tmp", "monitor", scenario, "test_*jobs.txt")))
if (all(os.path.exists(f) for f in monitorfiles) and monitorfiles):
    print("Found all files")
    df = pd.concat(
        [
            pd.read_table(
                f,
                delimiter="\s+",
                names=[
                    "USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", 
                    "COMMAND", "Platform file", "NJobs", "NFilesPerJob", "FileSize", "Hitrate"
                    ],
                )
        for f in monitorfiles],
        ignore_index=True
        )
    print("Simulation monitoring information: \n", df)
else:
    print("Couldn't find any files")
    exit(1)


# postprocess data frame
df['TIME'] = df['TIME'].str.split(":",expand=True).pipe(converttime, 0, 1)
df['RSS'] = df['RSS']/(1024*1024)
runtimes = df.loc[df.groupby("NJobs")["TIME"].idxmax()]
print("Filtered data:\n", runtimes)


# Visualize the monitoring information
fig, ax = plt.subplots()
ax.set_title("Simulation scaling " + scenario_plotlabel_dict[scenario])

# ax.set_xscale('log')
ax.set_xlabel('$N_{jobs}$', loc='right')
ax.set_ylabel('time / s')
# ax.set_xlim([0,2500])
# ax.set_ylim([0,400])

ax.plot(runtimes['NJobs'], runtimes['TIME'], linestyle='dotted', color='black')
ax.scatter(runtimes['NJobs'], runtimes['TIME'], color='black', marker='x', label='runtime')
ax.grid(axis="y", linestyle = 'dotted', which='major')

secax = ax.twinx()
secax.plot(runtimes['NJobs'], runtimes['RSS'],linestyle='dotted', color='orange')
secax.scatter(runtimes['NJobs'], runtimes['RSS'], color='orange', marker='^', label='memory')
# secax.xaxis.set_minor_locator(AutoMinorLocator())
secax.set_ylabel('memory / GiB')

h1, l1 = ax.get_legend_handles_labels()
h2, l2 = secax.get_legend_handles_labels()
ax.legend(h1+h2, l1+l2, loc=2)

fig.savefig("scalingtest_"+ scenario +".pdf")

# plt.show()