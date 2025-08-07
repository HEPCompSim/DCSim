#! /usr/bin/python3

import pandas as pd
from matplotlib import lines, pyplot as plt, patches as mpatches
import seaborn as sns
import os.path
import argparse
import re
import glob

from typing import Any
from collections import OrderedDict

import logging
logger = logging.getLogger('harvestVariationRuns')


QUANTITIES : dict[str, dict[str, Any]] = {
    "Walltime": {
        "ident": "Walltime",
        "label": "jobtime / min",
        # "ylim": [0.,7000.],
    },
    "IOtime": {
        "ident": "IOtime",
        "label": "transfer time / min",
        # "ylim": [0.,3500.],
    },
    "CPUtime": {
        "ident": "CPUtime",
        "label": "CPU time / min",
        # "ylim": [0.,5500.]
    },
    "Efficiency": {
        "ident": "Efficiency",
        "label": "CPU efficiency",
        "ylim": [0,1.3],
    },
    "hitrate": {
        "ident": "hitrate",
        "label": "hitrate",
        # "ylim": [0.,1.05],
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


def valid_int(param: str) -> int:
    value = int(param)
    if not value > 0:
        raise argparse.ArgumentTypeError("Argument must be greater than zero!")
    return value


def mapHostToSite(test: str, mapping: 'dict[str,str]',):
    match = next((x for x in mapping.keys() if x in test), "")
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
        logger.debug(f"\tProcessing file {file}")
        if not os.path.exists(file):
            raise FileNotFoundError(f"Input {file} not found!")
        with open(file) as f:
            # read one data file
            data = pd.read_csv(f,sep=r",\s",engine='python')
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
            match = re.search(
                r'(?:[Hh]itrate|[Hh])([0-9]+(?:\.[0-9]*)?)', os.path.splitext(os.path.basename(f.name))[0]
            )
            if match:
                logger.debug(f"\tExtracted prefetch rate {match.group(1)} from file name {f.name}")
                df_tmp["prefetchrate"] = float(match.group(1))
            else:
                raise ValueError(f"Could not extract prefetch rate from file name {f.name}")
            df_tmp.columns = [".".join(a).strip(".") for a in df_tmp.columns.to_flat_index()]
            logger.debug("\tintermediate dataframe: ", type(df_tmp), df_tmp.shape, "\n", df_tmp)
        return df_tmp


def createDataframeFromCSVs(csvFiles: list[str], nprocs=None) -> pd.DataFrame:
    """Merge all data from individual CSV files into a single data-frame

    Args:
        csvFiles (list[str]): CSV file paths containing job data
        nprocs (int|None): number of concurrent processes to use for processing. If None, it will use half of the available CPU cores.

    Returns:
        DataFrame: merged data-frame containing all job data
    """
    if nprocs is None:
        cpu_count = os.cpu_count()
        nprocs = cpu_count / 2 if cpu_count else 1
    if nprocs > len(csvFiles):
        logger.warning(f"Number of processes {nprocs} is greater than number of files {len(csvFiles)}. Reducing to {len(csvFiles)}.")
        nprocs = len(csvFiles)

    # create a dataframe containing statistical moments of each run
    from multiprocessing import Pool
    pool = Pool(processes=int(nprocs))
    process_dict = {}
    logger.info(f"Analysing {len(csvFiles)} files with {int(nprocs)} concurrent processes")
    for file in csvFiles:
        if not os.path.exists(file):
            logger.warning(f"File {file} does not exist, skipping.")
            continue
        process_dict[file] = pool.apply_async(processFile, (file,))
    dfs = []
    for file, process in process_dict.items():
        dfs.append(process.get())
    pool.close()
    pool.join()
    logger.info("\tFinished analysing")
    # concatenate all dataframes
    df = pd.concat([df for df in dfs], ignore_index=True)
    logger.debug(f"Raw data: \n{df.head()}")
    return df


def scale_xticks(ax: plt.Axes, ticks: list[float]):
        """Helper function which sets the xticks to the according scaled positions

        Args: 
            ax (matplotlib.Axes): subplot to scale xticks
            ticks (list[float]): list of expected ticks (at least two values, lowest and highest tick)
        """
        scale = (ax.get_xlim()[-1]-ax.get_xlim()[0]-1)/(ticks[-1]-ticks[0])
        print(f"Scale {(ticks[0],ticks[-1])} with {scale} to end up with correct seaborn axis {ax.get_xlim()}")
        ax.set_xticks([scale*x for x in ticks])
        ax.set_xticklabels(["{:.1f}".format(x) for x in ticks])


def plotVariationbands(
    ax: plt.Axes,
    df: pd.DataFrame,
    quantity: str,
    sites: 'list[str]',
    title: str = "",
):
    """Plot the median and 25- and 75-quantiles with uncertainty bands 

    Args:
        ax (plt.Axes): Axes to plot on
        df (pd.DataFrame): Data containing median and quantiles for indexed simulation run
        quantity (str): Quantity identifier to plot
        sites (list[str]): Sites to group by
        title (str): Plot title
    """
    # plot
    logger.info(f"\tPlotting quantity {quantity}")
    palette = sns.color_palette("colorblind", n_colors=len(sites))    
    sns.lineplot(data=df, x="prefetchrate", y=(".".join((quantity,"median"))),
                 hue="Site", hue_order=sites,
                 estimator="mean", errorbar=("ci",95), n_boot=1000, seed=42,
                 linestyle="solid", err_style="band", palette=palette,
                 ax=ax)
    sns.lineplot(data=df, x="prefetchrate", y=(".".join((quantity,"q25"))),
                 hue="Site", hue_order=sites,
                 estimator="mean", errorbar=("ci",95), n_boot=1000, seed=42,
                 linestyle="dashed", err_style="band", palette=palette,
                 ax=ax)
    sns.lineplot(data=df, x="prefetchrate", y=(".".join((quantity,"q75"))),
                 hue="Site", hue_order=sites,
                 estimator="mean", errorbar=("ci",95), n_boot=1000, seed=42,
                 linestyle="dashdot", err_style="band", palette=palette,
                 ax=ax)
    ax.set_title(title)
    ax.set_xlabel("fraction of prefetched files in cache",color="black")
    ax.set_ylabel(QUANTITIES[quantity]["label"], color="black")
    if QUANTITIES[quantity]["ylim"]:
        ax.set_ylim(QUANTITIES[quantity]["ylim"])
    # manipulate legend
    handles, labels = ax.get_legend_handles_labels()
    by_label = OrderedDict(zip(labels, handles))
    by_label["25% quantile"] = lines.Line2D([0],[0],color="black", linestyle="dashed")
    by_label["25% quantile"].set_linewidth(1.)
    by_label.move_to_end("25% quantile", last=False)
    by_label["median"] = lines.Line2D([0],[0],color="black", linestyle="solid")
    by_label["median"].set_linewidth(1.)
    by_label.move_to_end("median", last=False)
    by_label["75% quantile"] = lines.Line2D([0],[0],color="black", linestyle="dashdot")
    by_label["75% quantile"].set_linewidth(1.)
    by_label.move_to_end("75% quantile", last=False)
    ax.legend(by_label.values(), by_label.keys(), ncol=2, handlelength=1, loc='best',frameon=False)


def plotBoxes(
    ax: plt.Axes,
    df: pd.DataFrame,
    quantity: str,
    sites: 'list[str]',
    title: str = ""
):
    """Plot a boxplot with median and 25- and 75-quantiles and whiskers 
    corresponding to 1.5 times the interquartile range

    Args:
        ax (plt.Axes): Axes to plot on
        df (pd.DataFrame): Data containing median and quantiles for indexed simulation run
        quantity (str): Quantity identifier to plot
        sites (list[str]): Sites to group by
        title (str): Plot title
    """
    # plot
    logger.info(f"\tPlotting real-world data quantity {quantity}")
    palette = sns.color_palette("colorblind", n_colors=len(sites))
    sns.boxplot(data=df, x="prefetchrate", y=quantity,
                hue="Site", hue_order=sites, label=None,
                orient="v", flierprops=dict(marker="x"), palette=palette,
                ax=ax)
    ax.set_title(title)
    ax.set_xlabel("fraction of prefetched files in cache",color="black")
    ax.set_ylabel(QUANTITIES[quantity]["label"], color="black")
    if QUANTITIES[quantity]["ylim"]:
        ax.set_ylim(QUANTITIES[quantity]["ylim"])
    # manipulate legend
    handles, labels = ax.get_legend_handles_labels()
    by_label = OrderedDict(zip(labels, handles))
    by_label["data"] = mpatches.Patch(color="black")
    by_label.move_to_end("data", last=False)
    ax.legend(by_label.values(), by_label.keys(), ncol=2, handlelength=1, loc='best', frameon=False)


def run(args: argparse.Namespace):
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

    # Get list of files to process
    sim_files_to_process = []
    if args.sim_input_dir:
        if args.simfiles:
            logger.error("Cannot specify both --sim-input-dir and --simfiles.")
            exit(1)
        if not os.path.isdir(args.sim_input_dir):
            logger.error(f"Input directory not found: {args.sim_input_dir}")
            exit(1)
        logger.info(f"Searching for *.csv files in {args.sim_input_dir}")
        sim_files_to_process = glob.glob(os.path.join(args.sim_input_dir, "*.csv"))
    elif args.simfiles:
        sim_files_to_process = args.simfiles
    else:
        parser.print_help()
        logger.warning("\nNo simulation input files specified. Provide --simfiles or use --sim-input-dir.")

    data_files_to_process = []
    if args.data_input_dir:
        if args.datafiles:
            logger.error("Cannot specify both --data-input-dir and --datafiles.")
            exit(1)
        if not os.path.isdir(args.data_input_dir):
            logger.error(f"Input directory not found: {args.data_input_dir}")
            exit(1)
        logger.info(f"Searching for *.csv files in {args.data_input_dir}")
        data_files_to_process = glob.glob(os.path.join(args.data_input_dir, "*.csv"))
    if args.datafiles:
        data_files_to_process = args.datafiles

    if not sim_files_to_process and not data_files_to_process:
        logger.error("No files to process. Exiting.")
        exit(1)

    sim_df = pd.DataFrame()
    data_df = pd.DataFrame()
    sites = []
    if not sim_files_to_process:
        logger.warning("No simulation files to process.")
    else:
        logger.info(f"Found {len(sim_files_to_process)} simulation files to process.")
        # actual data processing
        sim_df = createDataframeFromCSVs(sim_files_to_process, nprocs)
        if sim_df.empty:
            logger.warning("No simulation data processed.")
        sites = sorted(sim_df["Site"].unique())

    if not data_files_to_process:
        logger.warning("No real-world data files to process.")
    else:
        logger.info(f"Found {len(data_files_to_process)} real-world data files to process.")
        # actual data processing
        data_df = createDataframeFromCSVs(data_files_to_process, nprocs)
        if data_df.empty:
            logger.warning("No real-world data processed.")
        # merge simulation and real-world data sites
        if sites:
            sites = sorted(set(sites) | set(data_df["Site"].unique()))
        else:
            sites = sorted(data_df["Site"].unique())

    # create output
    out_dir = os.path.abspath(args.out)
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    # and plot
    for quantity in QUANTITIES.values():
        logger.info("Plotting {}".format(quantity["ident"]))
        prefix = ""
        suffix = ""
        if prefix:
            prefix = prefix+"_"
        if suffix:
            suffix = "_"+suffix
        figsize = (6, 4)
        fig = plt.figure(f"{prefix}{quantity}{suffix}", figsize=figsize)
        ax1 = fig.add_subplot(1,1,1)
        plotVariationbands(ax1, sim_df, quantity["ident"], sites)
        plotBoxes(ax1, data_df, quantity["ident"], sites)
        # save plot
        fig.savefig(os.path.join(out_dir, f"{fig.get_label()}.pdf"))
        fig.savefig(os.path.join(out_dir, f"{fig.get_label()}.png"))
        plt.close()


if __name__ == "__main__":
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
        "--simfiles",
        nargs='*',
        type=valid_file,
        help="CSV monitor files from simulation to analyze. \
            Information about the simulated jobs \
            produced by the simulator."
    )
    parser.add_argument(
        "--sim-input-dir",
        type=str,
        help="Directory containing the monitor CSV files from simulation to analyze. \
            Cannot be used with --simfiles argument."
    )
    parser.add_argument(
        "--datafiles",
        nargs='*',
        type=valid_file,
        help="CSV monitor files from real-world data to analyze. \
            Information about run jobs \
            in a real-world experiment platform."
    )
    parser.add_argument(
        "--data-input-dir",
        type=str,
        help="Directory containing the monitor CSV files from real-world experimentation to analyze. \
            Cannot be used with --datafiles argument."
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
    args=parser.parse_args()
    run(args)

