#! /usr/bin/python3

import pandas as pd
import numpy as np
import matplotlib as mpl
from matplotlib import pyplot as plt
import seaborn as sns
import os.path
import argparse
import re

from collections.abc import Iterable
from collections import OrderedDict

import logging
logger = logging.getLogger('harvestVariationRuns')


QUANTITIES = {
    "Walltime": {
        "ident": "Walltime",
        "label": "jobtime / min",
        "ylim": [0.,7000.],
    },
    "IOtime": {
        "ident": "IOtime",
        "label": "transfer time / min",
        "ylim": [0.,3500.],
    },
    "CPUtime": {
        "ident": "CPUtime",
        "label": "CPU time / min",
        "ylim": [0.,5500.]
    },
    "Efficiency": {
        "ident": "Efficiency",
        "label": "CPU efficiency",
        "ylim": [0,1.3],
    },
    "hitrate": {
        "ident": "hitrate",
        "label": "hitrate",
        "ylim": [0.,1.05],
    }
}


HostSiteMapping = {
    "Tier1": "Tier 1",
    "Tier2": "Tier 2'",
}


def valid_file(param: str) -> str:
    base, ext = os.path.splitext(param)
    if ext.lower() not in (".csv"):
        raise argparse.ArgumentTypeError("File must have a csv extension")
    if not os.path.exists(param):
        raise FileNotFoundError('{}: No such file'.format(param))
    return param


def valid_int(param: int) -> int:
    param = int(param)
    if not param > 0:
        raise argparse.ArgumentTypeError("Argument must be greater than zero!")
    return param


parser = argparse.ArgumentParser(
    description="Produce a table (CSV file format) including the hitrate dependency of the simulated system for varied runs. \
        It uses several files as input, one for each prefetch value used to initialize the simulation per variation run. \
        These files containing the simulation dumps are CSV files produced by the output method of the simulator.",
    add_help=True
)
parser.add_argument(
    "--suffix",
    type=str,
    default="",
    help="Optonal string to add to the output-file name."
)
parser.add_argument(
    "monitorfiles",
    nargs='+',
    type=valid_file,
    help="CSV files containing the data to analyze. \
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
parser.add_argument(
    "--njobs", "-j",
    type=valid_int,
    help="number of concurrent jobs processing monitoring files"
)


def mapHostToSite(test: str, mapping: 'dict[str,str]',):
    match = next((x for x in mapping.keys() if x in test), False)
    if match:
        return mapping[match]
    else:
        return test



def q10(x: pd.Series):
    return x.quantile(0.1)
def q25(x: pd.Series):
    return x.quantile(0.25)
def q75(x: pd.Series):
    return x.quantile(0.75)
def q90(x: pd.Series):
    return x.quantile(0.9)


def processFile(file: os.PathLike):
        if not os.path.exists(file):
            raise FileNotFoundError(f"Input {file} not found!")
        with open(file) as f:
            # read one data file
            data = pd.read_csv(f,sep=",\s",engine='python')
            mask = ~data["job.tag"].str.contains("__")
            data = data[mask]
            # compute derived quantities
            data["Walltime"] = (data["job.end"]-data["job.start"])/60
            data["CPUtime"] = data["job.computetime"]/60
            data["IOtime"] = (data["infiles.transfertime"]+data["outfiles.transfertime"])/60
            data["Efficiency"] = data["job.computetime"]/(data["job.end"]-data["job.start"])
            data["Site"] = data["machine.name"].apply(lambda x: mapHostToSite(x,HostSiteMapping))
            # aggregate per execution site
            df_tmp = data.drop(columns=["job.tag","machine.name"]).groupby("Site").agg(['mean','median', q10, q25, q75, q90])
            df_tmp = df_tmp.reset_index()
            df_tmp["prefetchrate"] = float(re.search(r'([h,H])([0-9]*[.])?[0-9]*', os.path.splitext(os.path.basename(f.name))[0].split("_")[-2]).group().strip("hH"))
            df_tmp.columns = [".".join(a).strip(".") for a in df_tmp.columns.to_flat_index()]
            logger.debug("intermediate dataframe: ", type(df_tmp), df_tmp.shape, "\n", df_tmp)
        return df_tmp


def createDataframeFromCSVs(csvFiles: Iterable, nprocs=os.cpu_count()/2) -> pd.DataFrame:
    """Merge all data from individual CSV files into a single data-frame

    Args:
        csvFiles (List(PathLike)): CSV files containing job data
        nprocs (int): number of concurrent processes

    Returns:
        DataFrame: merged data-frame containing all job data
    """

    # create a dataframe containing statistical moments of each run
    from multiprocessing import Pool
    pool = Pool(processes=int(nprocs))
    process_dict = {}
    logger.info(f"Analysing {len(csvFiles)} with {nprocs} concurrent processes")
    for file in csvFiles:
        process_dict[file] = pool.apply_async(processFile, (file,))
    dfs = []
    for file, process in process_dict.items():
        dfs.append(process.get())
    pool.close()
    pool.join()
    # concatenate all dataframes
    df = pd.concat([df for df in dfs], ignore_index=True)
    logger.debug(f"Raw data: \n{df.head()}")
    return df



def plotVariationbands(
    df: pd.DataFrame,
    quantity: str,
    sites: 'list[str]',
    title: str,
    plot_dir: os.PathLike,
    prefix="", suffix="",
    figsize=(6,4)
):
    """Plot the median and 25- and 75-quantiles with uncertainty bands 

    Args:
        df (pd.DataFrame): Data containing median and quantiles for indexed simulation run
        quantity (str): Quantity identifier to plot
        sites (list[str]): Sites to group by
        title (str): Plot title
        plot_dir (os.PathLike): Save path for the plot
        prefix (str, optional): Prefix for plot name. Defaults to "".
        suffix (str, optional): Suffix for plot name. Defaults to "".
        figsize (tuple, optional): Figure aspect ratio. Defaults to (6,4).
    """

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

    # create plot output path
    plot_dir = os.path.abspath(plot_dir)
    if not os.path.exists(plot_dir):
        os.makedirs(plot_dir)
    if prefix:
        prefix = prefix+"_"
    if suffix:
        suffix = "_"+suffix
    # plot
    logger.info(f"\tPlotting quantity {quantity}")
    fig = plt.figure(f"{prefix}{quantity}{suffix}", figsize=figsize)
    ax1 = fig.add_subplot(1,1,1)
    palette = sns.color_palette("colorblind", n_colors=len(sites))    
    sns.lineplot(data=df, x="prefetchrate", y=(".".join((quantity,"median"))),
                 hue="Site", hue_order=sites,
                 estimator="mean", errorbar=("ci",95), err_style="band",
                 linestyle="solid", palette=palette,
                 ax=ax1)
    sns.lineplot(data=df, x="prefetchrate", y=(".".join((quantity,"q25"))),
                 hue="Site", hue_order=sites,
                 estimator="mean", errorbar=("ci",95), err_style="band",
                 linestyle="dashed", palette=palette,
                 ax=ax1)
    sns.lineplot(data=df, x="prefetchrate", y=(".".join((quantity,"q75"))),
                 hue="Site", hue_order=sites,
                 estimator="mean", errorbar=("ci",95), err_style="band",
                 linestyle="dashdot", palette=palette,
                 ax=ax1)
    ax1.set_title(title)
    ax1.set_xlabel("fraction of prefetched files in cache",color="black")
    ax1.set_ylabel(QUANTITIES[quantity]["label"], color="black")
    if QUANTITIES[quantity]["ylim"]:
        ax1.set_ylim(QUANTITIES[quantity]["ylim"])
    # manipulate legend
    handles, labels = ax1.get_legend_handles_labels()
    by_label = OrderedDict(zip(labels, handles))
    by_label["25% quantile"] = mpl.lines.Line2D([0],[0],color="black", linestyle="dashed")
    by_label["25% quantile"].set_linewidth(1.)
    by_label.move_to_end("25% quantile", last=False)
    by_label["median"] = mpl.lines.Line2D([0],[0],color="black", linestyle="solid")
    by_label["median"].set_linewidth(1.)
    by_label.move_to_end("median", last=False)
    by_label["75% quantile"] = mpl.lines.Line2D([0],[0],color="black", linestyle="dashdot")
    by_label["75% quantile"].set_linewidth(1.)
    by_label.move_to_end("75% quantile", last=False)
    ax1.legend(by_label.values(), by_label.keys(), ncol=2, handlelength=1, loc='best',frameon=False)
    # save plot
    fig.savefig(os.path.join(plot_dir, f"{fig.get_label()}.pdf"))
    fig.savefig(os.path.join(plot_dir, f"{fig.get_label()}.png"))
    plt.close()


def run(args=parser.parse_args()):
    # configure logging
    # pd.set_option('display.max_columns',None)
    logger.setLevel(getattr(logging, str(args.log).upper()))
    ch = logging.StreamHandler()
    ch.setLevel(getattr(logging, str(args.log).upper()))
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    ch.setFormatter(formatter)
    logger.addHandler(ch)

    nprocs = 1
    if args.njobs:
        nprocs = args.njobs

    # actual data processing
    df = createDataframeFromCSVs(args.monitorfiles, nprocs)
    sites = sorted(df["Site"].unique())

    # create output
    out_dir = os.path.abspath(args.out)
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    # and plot
    for quantity in QUANTITIES.values():
        logger.info("Plotting {}".format(quantity["ident"]))
        plotVariationbands(df, quantity["ident"], sites, "", out_dir)


if __name__ == "__main__":
    run()

