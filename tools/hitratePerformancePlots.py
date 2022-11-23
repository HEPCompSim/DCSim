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
plt.rcParams['axes.spines.left'] = True
plt.rcParams['axes.spines.right'] = False
plt.rcParams['axes.spines.top'] = False
plt.rcParams['axes.spines.bottom'] = True
plt.rcParams['axes.grid'] = False
plt.rcParams['axes.grid.axis'] = 'both'
plt.rcParams['axes.labelcolor'] = '#555555'
plt.rcParams['text.color'] = 'black'
plt.rcParams['figure.figsize'] = 6,4
plt.rcParams['figure.dpi'] = 100
plt.rcParams['figure.titleweight'] = 'normal'
plt.rcParams['font.family'] = 'sans-serif'


QUANTITIES = {
    "Walltime": {
        "ylabel": "jobtime / min",
        "ylim": None, # [20,65],
    },
    "IOtime": {
        "ylabel": "transfer time / min",
        "ylim": None, # [20,65],
    },
    "Efficiency": {
        "ylabel": "CPU eff.",
        "ylim": [0,1.05],
    },
}


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
    "--style",
    choices=["scatter", "boxplot","boxenplot", "jointplot"],
    default="scatter",
    type=str,
    help="Plot style for the visualization. Choose between scatterplot, boxplot, boxenplot or jointplot."
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
suffix="_"+args.suffix
plotstyle=args.style


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
machines = sorted(df["machine.name"].unique())
print(f"Unique machines for hue: {machines}")

for quantity, qstyle in QUANTITIES.items():
    print(f"Plotting for quantity {quantity}")
    if plotstyle == "scatter":
        fig = plt.figure(f"hitrate-{quantity}", figsize=(6,4))
        ax1 = sns.scatterplot(
            x="hitrate", y=quantity,
            hue="machine.name", hue_order=machines,
            data=df,
            palette=sns.color_palette("husl", len(machines)),
            alpha=0.9
        )
        ax1.set_title(scenario_plotlabel_dict[scenario])
        ax1.set_xlabel("hitrate", loc="right")
        ax1.set_ylabel(qstyle["ylabel"], color="black")
        ax1.set_xlim([-0.05,1.05])
        if qstyle["ylim"]:
            ax1.set_ylim(qstyle["ylim"])
        ax1.legend(loc='best')
        fig.savefig(f"hitrate{quantity}_{scenario}jobs{suffix}.pdf")
        fig.savefig(f"hitrate{quantity}_{scenario}jobs{suffix}.png")
        plt.close()

    elif plotstyle=="boxplot":
        fig = plt.figure(f"hitrate-{quantity}", figsize=(6,4))
        ax1 = sns.boxplot(
            x="hitrate", y=quantity,
            hue="machine.name", hue_order=machines,
            data=df,
            orient="v",
            palette=sns.color_palette("husl", len(machines))
        )
        ax1.set_title(scenario_plotlabel_dict[scenario])
        ax1.set_xlabel("hitrate", loc="right")
        ax1.set_ylabel(qstyle["ylabel"], color="black")
        if qstyle["ylim"]:
            ax1.set_ylim(qstyle["ylim"])
        ax1.legend(loc='best')
        fig.savefig(f"hitrate{quantity}_{scenario}jobs{suffix}.pdf")
        fig.savefig(f"hitrate{quantity}_{scenario}jobs{suffix}.png")
        plt.close()

    elif plotstyle =="boxenplot":
        fig = plt.figure("hitrate-walltime", figsize=(6,4))
        ax1 = sns.boxenplot(
            x="hitrate", y=quantity,
            hue="machine.name", hue_order=machines,
            data=df,
            orient="v",
            palette=sns.color_palette("husl", len(machines))
        )
        ax1.set_title(scenario_plotlabel_dict[scenario])
        ax1.set_xlabel("hitrate", loc="right")
        ax1.set_ylabel(qstyle["ylabel"], color="black")
        if qstyle["ylim"]:
            ax1.set_ylim(qstyle["ylim"])
        ax1.legend(loc='best')
        fig.savefig(f"hitrate{quantity}_{scenario}jobs{suffix}.pdf")
        fig.savefig(f"hitrate{quantity}_{scenario}jobs{suffix}.png")
        plt.close()

    elif plotstyle == "jointplot":
        grid = sns.jointplot(
            x="hitrate", y=quantity,
            hue="machine.name", hue_order=machines,
            data=df,
            xlim=[-0.05,1.05],
            ylim=qstyle["ylim"] if qstyle["ylim"] else None,
            kind="kde", marginal_ticks=True,
            height=7,
            palette=sns.color_palette("husl", len(machines))
        )
        grid.set_axis_labels(xlabel="hitrate", ylabel=qstyle["ylabel"], color="black")
        grid.savefig(f"hitrate{quantity}_{scenario}jobs{suffix}.pdf")
        grid.savefig(f"hitrate{quantity}_{scenario}jobs{suffix}.png")
        plt.close()

    else:
        raise NotImplementedError(f"Plotstyle {plotstyle} not implemented") 
