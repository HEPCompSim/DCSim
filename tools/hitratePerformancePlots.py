#! /usr/bin/python3

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import seaborn as sns
import os.path
import argparse
from collections.abc import Iterable


plt.rcParams['figure.autolayout'] = True
pd.set_option('display.max_columns',None)
plt.rcParams['axes.facecolor'] = 'white'
plt.rcParams['axes.spines.left'] = True
plt.rcParams['axes.spines.right'] = False
plt.rcParams['axes.spines.top'] = False
plt.rcParams['axes.spines.bottom'] = True
plt.rcParams['axes.grid'] = False
plt.rcParams['axes.grid.axis'] = 'both'
plt.rcParams['axes.labelcolor'] = 'black'
plt.rcParams['axes.labelsize'] = 13
plt.rcParams['text.color'] = 'black'
plt.rcParams['figure.figsize'] = 6,4
plt.rcParams['figure.dpi'] = 100
plt.rcParams['figure.titleweight'] = 'normal'
plt.rcParams['font.family'] = 'sans-serif'
# plt.rcParams['font.weight'] = 'bold'
plt.rcParams['font.size'] = 13

markers = ['o', '+', 'x', '*', '.', 'X']

QUANTITIES = {
    "Walltime": {
        "ylabel": "jobtime / min",
        "ylim": [0,300], # [0,65],
    },
    "IOtime": {
        "ylabel": "transfer time / min",
        "ylim": None, # [0,65],
    },
    "CPUtime": {
        "ylabel": "CPU time / min",
        "ylim": None,
    },
    "Efficiency": {
        "ylabel": "CPU eff.",
        "ylim": [0,1.05],
    },
}


scenario_plotlabel_dict = {
    "": "",
    "data": "Data",
    "copy": "Input-files copied",
    "fullstream": "Block-streaming",
    "SGBatch_fullstream_10G": "SG-Batch 10Gb/s gateway",
    "SGBatch_fullstream_1G": "SG-Batch 1Gb/s gateway",
    "SGBatch_fullstream_10G_70Mcache": "SG-Batch 10Gb/s gateway 70MB/s cache"
}


def valid_file(param: str) -> str:
    base, ext = os.path.splitext(param)
    if ext.lower() not in (".csv"):
        raise argparse.ArgumentTypeError("File must have a csv extension")
    if not os.path.exists(param):
        raise FileNotFoundError('{}: No such file'.format(param))
    return param


def scale_xticks(ax: plt.Axes, ticks: Iterable):
    """Helper function which sets the xticks to the according scaled positions

    Args: 
        ax (matplotlib.Axes): subplot to scale xticks
        ticks (Iterable): list of expected ticks (at least two values, lowest and highest tick)
    """
    scale = (ax1.get_xlim()[-1]-ax1.get_xlim()[0]-1)/(ticks[-1]-ticks[0])
    print(f"Scale {(ticks[0],ticks[-1])} with {scale} to end up with correct seaborn axis {ax.get_xlim()}")
    ax.set_xticks([scale*x for x in ticks])
    ax.set_xticklabels(["{:.1f}".format(x) for x in ticks])


def mapHostToSite(test: str, mapping: 'dict[str,str]',):
    match = next((x for x in mapping.keys() if x in test), False)
    if match:
        return mapping[match]
    else:
        return test


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
    default="",
    help="Choose a scenario, which is used in the according plotting label and file-name of the plot."
)
parser.add_argument(
    "--style",
    choices=["scatterplot", "pointplot", "boxplot", "boxenplot", "violinplot", "jointplot"],
    default="scatterplot",
    type=str,
    help="Plot style for the visualization."
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

if args.scenario != "":
    scenario = "_"+args.scenario.lower()
else:
    scenario = ""
if args.suffix:
    suffix = "_"+args.suffix
else:
    suffix = ""
plotstyle = args.style


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
df["CPUtime"] = df["job.computetime"]/60
df["Efficiency"] = df["job.computetime"]/(df["job.end"]-df["job.start"])
mapping = {
    "sg01": "Worker Node 1",
    "sg03": "Worker Node 2",
    "sg04": "Worker Node 3",
}
df["Site"] = df["machine.name"].apply(lambda x: mapHostToSite(x,mapping))


# plot and save
machines = sorted(df["machine.name"].unique())
sites = sorted(df["Site"].unique())
print(f"Unique machines for hue: {machines}")
print(f"Unique sites for hue: {sites}")
hitrateticks = [x*0.1 for x in range(0,11)]

for quantity, qstyle in QUANTITIES.items():

    print(f"Plotting for quantity {quantity}")

    if plotstyle == "scatterplot":
        fig = plt.figure(f"hitrate-{quantity}", figsize=(6,4))
        ax1 = sns.scatterplot(
            x="hitrate", y=quantity,
            hue="Site", hue_order=sites,
            data=df,
            palette=sns.color_palette("colorblind", n_colors=len(sites)),
            alpha=0.9
        )
        ax1.set_title(scenario_plotlabel_dict[scenario])
        ax1.set_xlabel("hitrate", loc="right")
        ax1.set_ylabel(qstyle["ylabel"], color="black")
        ax1.set_xlim([-0.05,1.05])
        ax1.set_xticks(hitrateticks)
        if qstyle["ylim"]:
            ax1.set_ylim(qstyle["ylim"])
        ax1.legend(loc='best')
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.pdf")
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.png")
        plt.close()

    elif plotstyle == "pointplot":
        markers = markers[0:len(sites)]
        fig = plt.figure(f"hitrate-{quantity}", figsize=(6,4))
        ax1 = fig.add_subplot(1,1,1)
        sns.pointplot(
            x="hitrate", y=quantity,
            hue="Site", hue_order=sites,
            data=df,
            estimator="median", errorbar=("pi",95), # ci = Confidence Interval, pi = Percentile Interval, sd = Standard Deviation, se = Standard Error of Mean
            dodge=True, join=False,
            markers=markers, capsize=0.5/len(sites), errwidth=1.,
            palette=sns.color_palette("colorblind", n_colors=len(sites)),
            ax=ax1
        )
        ax1.set_title(scenario_plotlabel_dict[scenario])
        ax1.set_xlabel("hitrate", loc="right", color="black")
        scale_xticks(ax1, hitrateticks)
        ax1.set_ylabel(qstyle["ylabel"], color="black")
        if qstyle["ylim"]:
            ax1.set_ylim(qstyle["ylim"])
        ax1.legend(loc='best')
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.pdf")
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.png")
        plt.close()

    elif plotstyle=="boxplot":
        fig = plt.figure(f"hitrate-{quantity}", figsize=(6,4))
        ax1 = sns.boxplot(
            x="hitrate", y=quantity,
            hue="Site", hue_order=sites,
            data=df,
            orient="v",
            # whis=1.5, #[0.5,99.5],
            flierprops=dict(marker="x"),
            palette=sns.color_palette("colorblind", n_colors=len(sites)),
        )
        ax1.set_title(scenario_plotlabel_dict[scenario])
        ax1.set_xlabel("hitrate", loc="right")
        scale_xticks(ax1, hitrateticks)
        ax1.set_ylabel(qstyle["ylabel"], color="black")
        if qstyle["ylim"]:
            ax1.set_ylim(qstyle["ylim"])
        ax1.legend(loc='best')
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.pdf")
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.png")
        plt.close()

    elif plotstyle =="boxenplot":
        fig = plt.figure("hitrate-walltime", figsize=(6,4))
        ax1 = sns.boxenplot(
            x="hitrate", y=quantity,
            hue="Site", hue_order=sites,
            data=df,
            orient="v",
            k_depth="proportion",
            linewidth=0.5,
            flier_kws=dict(marker="."),
            palette=sns.color_palette("colorblind", n_colors=len(sites)),
        )
        ax1.set_title(scenario_plotlabel_dict[scenario])
        ax1.set_xlabel("hitrate", loc="right")
        scale_xticks(ax1, hitrateticks)
        ax1.set_ylabel(qstyle["ylabel"], color="black")
        if qstyle["ylim"]:
            ax1.set_ylim(qstyle["ylim"])
        ax1.legend(loc='best')
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.pdf")
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.png")
        plt.close()

    elif plotstyle =="violinplot":
        fig = plt.figure("hitrate-walltime", figsize=(6,4))
        ax1 = sns.violinplot(
            x="hitrate", y=quantity,
            hue="Site", hue_order=sites,
            data=df,
            orient="v",
            bw="scott", scale="count", inner="quartile",
            linewidth=0.5,
            palette=sns.color_palette("colorblind", n_colors=len(sites)),
        )
        ax1.set_title(scenario_plotlabel_dict[scenario])
        ax1.set_xlabel("hitrate", loc="right")
        scale_xticks(ax1, hitrateticks)
        ax1.set_ylabel(qstyle["ylabel"], color="black")
        if qstyle["ylim"]:
            ax1.set_ylim(qstyle["ylim"])
        ax1.legend(loc='best')
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.pdf")
        fig.savefig(f"hitrate{quantity}{scenario}{suffix}.png")
        plt.close()

    elif plotstyle == "jointplot":
        grid = sns.JointGrid(
            x="hitrate", y=quantity,
            hue="Site", hue_order=sites,
            data=df,
            xlim=[-0.1,1.1],
            ylim=qstyle["ylim"] if qstyle["ylim"] else None,
            marginal_ticks=True,
            height=7,
            palette=sns.color_palette("colorblind", n_colors=len(sites)),
        )
        grid.plot_joint(sns.scatterplot)
        grid.plot_marginals(sns.histplot, multiple="layer", element="step", fill=True)

        grid.set_axis_labels(xlabel="hitrate", ylabel=qstyle["ylabel"], color="black")
        grid.savefig(f"hitrate{quantity}{scenario}{suffix}.pdf")
        grid.savefig(f"hitrate{quantity}{scenario}{suffix}.png")
        plt.close()

    else:
        raise NotImplementedError(f"Plotstyle {plotstyle} not implemented") 
