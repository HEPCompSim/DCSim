#! /usr/bin/python3

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import seaborn as sns
import os.path
import argparse
from collections.abc import Iterable

import logging
logger = logging.getLogger('classifyWorkloads')


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
        "ident": "Walltime",
        "label": "jobtime / min",
        "ylim": [0.,4000.],
    },
    "IOtime": {
        "ident": "IOtime",
        "label": "transfer time / min",
        "ylim": [0.,3000.],
    },
    "CPUEfficiency": {
        "ident": "Efficiency",
        "label": "CPU eff.",
        "ylim": [0,1.05],
    },
    "Hitrate": {
        "ident": "hitrate",
        "label": "hitrate",
        "ylim": [0.,1.05],
    }
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
    scale = (ax.get_xlim()[-1]-ax.get_xlim()[0]-1)/(ticks[-1]-ticks[0])
    print(f"Scale {(ticks[0],ticks[-1])} with {scale} to end up with correct seaborn axis {ax.get_xlim()}")
    ax.set_xticks([scale*x for x in ticks])
    ax.set_xticklabels(["{:.1f}".format(x) for x in ticks])


parser = argparse.ArgumentParser(
    description="Produce plot showing the hitrate dependency of the simulated system. \
        It uses several files, one for each prefetch value used to initialize the simulation. \
        The files containing the simulation dump are CSV files produced by the output method of the simulator.",
    add_help=True
)
parser.add_argument(
    "--scenario", 
    type=str,
    default="",
    help="String which will be used as a label to distinguish different scenarios"
)
parser.add_argument(
    "--suffix",
    type=str,
    default="",
    help="Optonal string to add to the file-name of the plot."
)
parser.add_argument(
    "monitorfiles",
    nargs='+',
    type=valid_file,
    help="CSV files containing the data to plot. \
        Information about the simulated jobs \
        produced by the simulator."
)
parser.add_argument(
    "--out","-o",
    default=os.path.join(os.path.dirname(__file__),".."),
    help="path to the directory where outputs should be dumped"
)
parser.add_argument(
    "--log",
    choices=("info", "debug", "warning", "error", "critical"),
    default="info",
    help="set the logging level",
)


def createDataframeFromCSVs(csvFiles: Iterable):
    """Merge all data from individual CSV files into a single data-frame

    Args:
        csvFiles (List(PathLike)): CSV files containing job data

    Returns:
        DataFrame: merged data-frame containing all job data
    """
    if all(os.path.exists(f) for f in csvFiles):
        # create a dataframe for each CSV file
        dfs = []
        for ifile, file in enumerate(csvFiles):
            with open(file) as f:
                df_tmp = pd.read_csv(f,sep=",\s")
                df_tmp["prefetchrate"] = ifile*0.1
                dfs.append(df_tmp)
        # concatenate all dataframes
        df = pd.concat([df for df in dfs], ignore_index=True)
        logger.debug(f"Raw data: \n{df.head()}")
    else:
        logger.error("Couldn't find CSV files")
    return df


def plotHistograms(
    df: pd.DataFrame,
    sites: 'list[str]',
    title="",
    quantities=QUANTITIES,
    prefix="", suffix="",
    figsize=(6,4),
    plot_dir=os.path.join(os.path.dirname(__file__),"..","plots")
):
    plot_dir = os.path.abspath(plot_dir)
    if not os.path.exists(plot_dir):
        os.makedirs(plot_dir)
    if prefix:
        prefix = prefix+"_"
    if suffix:
        suffix = "_"+suffix
    for kquantity, dquantity in quantities.items():
        logger.debug(f"\tPlotting quantity {kquantity}")
        fig = plt.figure(f"{prefix}{kquantity}{suffix}", figsize=figsize)
        ax1 = sns.boxplot(
            x="prefetchrate", y=dquantity["ident"],
            hue="Site", hue_order=sites,
            data=df,
            orient="v",
            # whis=1.5, #[0.5,99.5],
            flierprops=dict(marker="x"),
            palette=sns.color_palette("colorblind", n_colors=len(sites))
        )
        ax1.set_title(title)
        ax1.set_xlabel("prefetch-rate",color="black")
        hitrateticks = [x*0.1 for x in range(0,11)]
        scale_xticks(ax1, hitrateticks)
        ax1.set_ylabel(dquantity["label"], color="black")
        if dquantity["ylim"]:
            ax1.set_ylim(dquantity["ylim"])
        ax1.legend(loc='best')
        fig.savefig(os.path.join(plot_dir, f"{fig.get_label()}.pdf"))
        fig.savefig(os.path.join(plot_dir, f"{fig.get_label()}.png"))
        plt.close()


def mapHostToSite(test: str, mapping: 'dict[str,str]',):
    match = next((x for x in mapping.keys() if x in test), False)
    if match:
        return mapping[match]
    else:
        return test


def run(args=parser.parse_args()):
    # configure logging
    logger.setLevel(getattr(logging, str(args.log).upper()))
    ch = logging.StreamHandler()
    ch.setLevel(getattr(logging, str(args.log).upper()))
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    ch.setFormatter(formatter)
    logger.addHandler(ch)

    # actual data processing
    out_dir = os.path.abspath(args.out)
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    data = createDataframeFromCSVs(args.monitorfiles)
    print(data.keys())
    # Derive quantities
    data["Walltime"] = (data["job.end"]-data["job.start"])/60
    data["IOtime"] = (data["infiles.transfertime"]+data["outfiles.transfertime"])/60
    data["Efficiency"] = data["job.computetime"]/(data["job.end"]-data["job.start"])
    #TODO: classify according to sites
    mapping = {
        "Tier1": "T1",
        "Tier2": "T2",
    }
    data["Site"] = data["machine.name"].apply(lambda x: mapHostToSite(x,mapping))
    sites = sorted(data["Site"].unique())

    plotHistograms(data,
                   sites=sites,
                   title=args.scenario,
                   suffix=args.scenario)


if __name__ == "__main__":
    run()
