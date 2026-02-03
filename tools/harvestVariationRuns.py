#!/usr/bin/env python3

import pandas as pd
from matplotlib import lines, markers, pyplot as plt, patches as mpatches
import seaborn as sns
import os.path
import argparse
import re
import glob
import tarfile
import tempfile

from typing import Any, Callable
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
    if not param.endswith((".csv", ".tar.gz")):
        raise argparse.ArgumentTypeError("File must have a .csv or .tar.gz extension")
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


def processSimFile(file: os.PathLike):
        logger.debug(f"\tProcessing file {file}")
        if not os.path.exists(file):
            raise FileNotFoundError(f"Input {file} not found!")
        with open(file) as f:
            # read one data file
            try:
                data = pd.read_csv(f,sep=r"\s*,\s*", engine='python', index_col=False)
            except pd.errors.EmptyDataError as e:
                logger.error(f"Something is wrong with the input file {file}: {e}")
                return pd.DataFrame()
            # mask dummy simulation jobs that are tagged with "__"
            # these are not real jobs, but only used to simulate the prefetching
            # and are not relevant for the analysis
            mask = ~data["job.tag"].str.contains("__")
            data = data[mask]
            # compute derived quantities
            try:
                data["Walltime"] = (data["job.runtime"])/60
            except KeyError: # runtime is not available in older data sets
                data["Walltime"] = (data["job.end"]-data["job.start"])/60
            data["CPUtime"] = data["job.computetime"]/60
            data["IOtime"] = (data["infiles.transfertime"]+data["outfiles.transfertime"])/60
            try:
                data["Efficiency"] = data["job.computetime"]/(data["job.runtime"])
            except KeyError: # runtime is not available in older data sets
                data["Efficiency"] = data["job.computetime"]/(data["job.end"]-data["job.start"])
            data["Site"] = data["machine.name"].astype(str).apply(lambda x: mapHostToSite(x, HostSiteMapping))
            # aggregate per execution site
            df_tmp = data.drop(columns=["job.tag","machine.name"]).groupby("Site").agg(['mean','median', q10, q25, q75, q90])
            df_tmp = df_tmp.reset_index()
            match = re.search(
                r'(?:(?:[Hh]itrate|[Hh])_?([0-9]+(?:\.[0-9]*)?))|([0-9]+\.[0-9]+)', os.path.splitext(os.path.basename(f.name))[0]
            )
            if match:
                if match.group(2):
                    logger.warning(f"File name {f.name} uses deprecated format for prefetch rate extraction. Please use 'HitrateX.Y', 'hX.Y', 'H_X.Y' or similar format.")
                val = match.group(1) if match.group(1) else match.group(2)
                logger.debug(f"\tExtracted prefetch rate {val} from file name {f.name}")
                df_tmp["prefetchrate"] = float(val)
            else:
                raise ValueError(f"Could not extract prefetch rate from file name {f.name}")
            df_tmp.columns = [".".join(a).strip(".") for a in df_tmp.columns.to_flat_index()]
            logger.debug("\tintermediate dataframe: ", type(df_tmp), df_tmp.shape, "\n", df_tmp)
        return df_tmp

def processDataFile(file: os.PathLike):
        logger.debug(f"\tProcessing file {file}")
        if not os.path.exists(file):
            raise FileNotFoundError(f"Input {file} not found!")
        with open(file) as f:
            # read one data file
            try:
                data = pd.read_csv(f,sep=r"\s*,\s*", engine='python', index_col=False)
            except pd.errors.EmptyDataError as e:
                logger.error(f"Something is wrong with the input file {file}: {e}")
                return pd.DataFrame()
            # compute derived quantities
            try:
                data["Walltime"] = (data["job.runtime"])/60
            except KeyError: # runtime is not available in older data sets
                data["Walltime"] = (data["job.end"]-data["job.start"])/60
            data["CPUtime"] = data["job.computetime"]/60
            data["IOtime"] = -9999.9  # Placeholder for IO time, as it is not computed here
            try:
                data["Efficiency"] = data["job.computetime"]/(data["job.runtime"])
            except KeyError: # runtime is not available in older data sets
                data["Efficiency"] = data["job.computetime"]/(data["job.end"]-data["job.start"])
            data["Site"] = data["machine.name"].astype(str).apply(lambda x: mapHostToSite(x, HostSiteMapping))
            # Keep only the site and the columns to be aggregated
            cols_to_keep = ["Walltime", "CPUtime", "IOtime", "Efficiency", "Site", "hitrate"]
            df_for_agg = data[cols_to_keep]
            # aggregate per execution site
            cols_to_agg = ["Walltime", "CPUtime", "IOtime", "Efficiency", "hitrate"]
            df_tmp = df_for_agg.groupby("Site")[cols_to_agg].agg(['mean','median', q10, q25, q75, q90])
            df_tmp = df_tmp.reset_index()
            match = re.search(
                r'(?:(?:[Hh]itrate|[Hh])_?([0-9]+(?:\.[0-9]*)?))|([0-9]+\.[0-9]+)', os.path.splitext(os.path.basename(f.name))[0]
            )
            if match:
                if match.group(2):
                    logger.warning(f"File name {f.name} uses deprecated format for prefetch rate extraction. Please use 'HitrateX.Y', 'hX.Y', 'H_X.Y' or similar format.")
                val = match.group(1) if match.group(1) else match.group(2)
                logger.debug(f"\tExtracted prefetch rate {val} from file name {f.name}")
                df_tmp["prefetchrate"] = float(val)
            else:
                raise ValueError(f"Could not extract prefetch rate from file name {f.name}")
            df_tmp.columns = [".".join(a).strip(".") for a in df_tmp.columns.to_flat_index()]
            logger.debug("\tintermediate dataframe: ", type(df_tmp), df_tmp.shape, "\n", df_tmp)
        return df_tmp


def createDataframeFromCSVs(files: list[str], processor: Callable[[os.PathLike], pd.DataFrame], nprocs: int|None = None) -> pd.DataFrame:
    """Merge all data from individual CSV files into a single data-frame

    Args:
        csvFiles (list[str]): CSV file paths or tar.gz archives with .csv files containing job data
        nprocs (int|None): number of concurrent processes to use for processing. If None, it will use half of the available CPU cores.

    Returns:
        DataFrame: merged data-frame containing all job data
    """
    csv_files = []
    temp_dirs = []

    for file_path in files:
        if file_path.lower().endswith(".tar.gz"):
            tmpdir = tempfile.mkdtemp()
            temp_dirs.append(tmpdir)
            logger.info(f"Extracting {file_path} to {tmpdir}")
            with tarfile.open(file_path, "r:gz") as tar:
                tar.extractall(path=tmpdir)
            extracted_csvs = glob.glob(os.path.join(tmpdir, "**/*.csv"), recursive=True)
            csv_files.extend(extracted_csvs)
        elif file_path.lower().endswith(".csv"):
            csv_files.append(file_path)

    if not csv_files:
        for dr in temp_dirs:
            import shutil
            shutil.rmtree(dr)
        logger.error(f"No valid CSV files found in the provided file paths {files}.")
        return pd.DataFrame()
    
    if nprocs is None:
        cpu_count = os.cpu_count()
        nprocs = cpu_count // 2 if cpu_count else 1
    if nprocs > len(csv_files):
        logger.warning(f"Number of processes {nprocs} is greater than number of files {len(csv_files)}. Reducing to {len(csv_files)}.")
        nprocs = len(csv_files)

    # create a dataframe containing statistical moments of each run
    from multiprocessing import Pool
    pool = Pool(processes=int(nprocs))
    process_dict = {}
    logger.info(f"Analysing {len(csv_files)} files with {int(nprocs)} concurrent processes")
    for file in csv_files:
        if not os.path.exists(file):
            logger.warning(f"File {file} does not exist, skipping.")
            continue
        process_dict[file] = pool.apply_async(processor, (file,))
    dfs = []
    for file, process in process_dict.items():
        dfs.append(process.get())
    pool.close()
    pool.join()
    logger.info("\tFinished analysing")

    # Clean up temporary directories
    for dr in temp_dirs:
        import shutil
        shutil.rmtree(dr)

    # concatenate all dataframes
    if not dfs:
        logger.error(f"No valid dataframes created from the provided files {files}.")
        return pd.DataFrame()
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
    logger.info(f"\tPlotting simulated quantity {quantity}")
    # print(df.head())
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
    if "ylim" in QUANTITIES[quantity]:
        if QUANTITIES[quantity]["ylim"]:
            ax.set_ylim(QUANTITIES[quantity]["ylim"])


def plotBoxes(
    ax: plt.Axes,
    df: pd.DataFrame,
    quantity: str,
    sites: 'list[str]',
    title: str = ""
):
    """Plot data points with error bars based on pre-computed quantiles.

    Args:
        ax (plt.Axes): Axes to plot on
        df (pd.DataFrame): Data containing median and quantiles for indexed experiment run
        quantity (str): Quantity identifier to plot
        sites (list[str]): Sites to group by
        title (str): Plot title
    """
    # plot
    logger.info(f"\tPlotting real-world data quantity {quantity}")
    # print(df.head())
    palette = sns.color_palette("colorblind", n_colors=len(sites))
    
    # Define a small jitter width to offset points for different sites
    jitter_width = 0.01   
    for i, site in enumerate(sites):
        site_df = df[df['Site'] == site].sort_values('prefetchrate')
        if site_df.empty:
            continue

        # Apply jitter to the x-axis values
        grouped = site_df.groupby('prefetchrate')
        x_values = grouped['prefetchrate'].first() + (i * jitter_width)

        # Aggregate the y-values and variations
        y_values = grouped[f'{quantity}.median'].mean()
        # print(f"{quantity}.median.means for site {site}: {y_values}, total mean: {y_values.mean()}")
        lower_errors = y_values - grouped[f'{quantity}.q25'].mean()
        upper_errors = grouped[f'{quantity}.q75'].mean() - y_values
        y_err = [lower_errors.to_numpy(), upper_errors.to_numpy()]

        ax.errorbar(x=x_values, y=y_values, yerr=y_err, fmt='o', capsize=5, color=palette[i], label=site)

    ax.set_title(title)
    ax.set_xlabel("fraction of prefetched files in cache",color="black")
    ax.set_ylabel(QUANTITIES[quantity]["label"], color="black")
    if "ylim" in QUANTITIES[quantity]:
        if QUANTITIES[quantity]["ylim"]:
            ax.set_ylim(QUANTITIES[quantity]["ylim"])


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
        logger.info(f"Searching for *.csv and *tar.gz files in {args.sim_input_dir}")
        sim_files_to_process.extend(glob.glob(os.path.join(args.sim_input_dir, "*.csv")))
        sim_files_to_process.extend(glob.glob(os.path.join(args.sim_input_dir, "*.tar.gz")))
    elif args.simfiles:
        sim_files_to_process = args.simfiles
    else:
        parser.print_help()
        logger.warning("No simulation input files specified. Provide --simfiles or use --sim-input-dir.")

    data_files_to_process = []
    if args.data_input_dir:
        if args.datafiles:
            logger.error("Cannot specify both --data-input-dir and --datafiles.")
            exit(1)
        if not os.path.isdir(args.data_input_dir):
            logger.error(f"Input directory not found: {args.data_input_dir}")
            exit(1)
        logger.info(f"Searching for *.csv and *.tar.gz files in {args.data_input_dir}")
        data_files_to_process.extend(glob.glob(os.path.join(args.data_input_dir, "*.csv")))
        data_files_to_process.extend(glob.glob(os.path.join(args.data_input_dir, "*.tar.gz")))
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
        sim_df = createDataframeFromCSVs(sim_files_to_process, processSimFile, nprocs)
        if sim_df.empty:
            logger.warning("No simulation data processed.")
        sites = sorted(sim_df["Site"].unique())

    if not data_files_to_process:
        logger.warning("No real-world data files to process.")
    else:
        logger.info(f"Found {len(data_files_to_process)} real-world data files to process.")
        # actual data processing
        data_df = createDataframeFromCSVs(data_files_to_process, processDataFile, nprocs)
        if data_df.empty:
            logger.warning("No real-world data processed.")
        # merge simulation and real-world data sites
        if sites:
            sites = sorted(set(sites) | set(data_df["Site"].unique()))
        else:
            sites = sorted(data_df["Site"].unique())

    # create output
    out_dir = os.path.abspath(args.out)
    logger.info(f"Creating output in directory {out_dir}")
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    # and plot
    for quantity in QUANTITIES.values():
        logger.info("Plotting {}".format(quantity["ident"]))
        prefix = ""
        suffix = args.suffix
        if prefix:
            prefix = prefix+"_"
        if suffix:
            suffix = "_"+suffix
        figsize = (6, 4)
        fig = plt.figure(f"{prefix}{quantity}{suffix}", figsize=figsize)
        ax1 = fig.add_subplot(1,1,1)
        legend_dict = OrderedDict()
        if not sim_df.empty:
            plotVariationbands(ax1, sim_df, quantity["ident"], sites)
            _handles, _labels = ax1.get_legend_handles_labels()
            by_label = OrderedDict(zip(_labels, _handles))
            by_label["25% quantile"] = lines.Line2D([0],[0],color="black", linestyle="dashed")
            by_label["25% quantile"].set_linewidth(1.)
            by_label.move_to_end("25% quantile", last=False)
            by_label["median"] = lines.Line2D([0],[0],color="black", linestyle="solid")
            by_label["median"].set_linewidth(1.)
            by_label.move_to_end("median", last=False)
            by_label["75% quantile"] = lines.Line2D([0],[0],color="black", linestyle="dashdot")
            by_label["75% quantile"].set_linewidth(1.)
            by_label.move_to_end("75% quantile", last=False)
            legend_dict.update(by_label)
        if not data_df.empty:
            plotBoxes(ax1, data_df, quantity["ident"], sites)
            _handles, _labels = ax1.get_legend_handles_labels()
            by_label = OrderedDict(zip(_labels, _handles))
            by_label["data"] = lines.Line2D([0], [0], marker='o', linestyle='None', color="black")
            for key, value in by_label.items():
                if key not in legend_dict:
                    legend_dict[key] = value
            legend_dict.move_to_end("data", last=False)
        ax1.legend(legend_dict.values(), legend_dict.keys(), ncol=2, handlelength=1, loc='best', frameon=False)
        # save plot
        fig.savefig(os.path.join(out_dir, f"{quantity['ident']}{suffix}.pdf"))
        fig.savefig(os.path.join(out_dir, f"{quantity['ident']}{suffix}.png"))
        plt.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Produce a set of plots including the hitrate dependency of the simulated system for varied runs \
            that can be compared to data. \
            It uses several files as input, one for each prefetch value used to initialize the simulation per variation run. \
            Accordingly, the filename must indicate the prefetch rate used in the simulation run, e.g., 'Hitrate0.5', or 'h0.5' for a prefetch rate of 50%. \
            The files, containing the simulation dumps, are CSV files produced by the output method of the simulator.",
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
        help="CSV monitor files or zipped tar archives from simulation to analyze. \
            Information about the simulated jobs \
            produced by the simulator."
    )
    parser.add_argument(
        "--sim-input-dir",
        type=str,
        help="Directory containing the monitor CSV files or tar archives from simulation to analyze. \" \
            Having both the CSV files as well as the archives containing the same files in this directory \
            will lead to double counting! \
            Cannot be used with --simfiles argument."
    )
    parser.add_argument(
        "--datafiles",
        nargs='*',
        type=valid_file,
        help="CSV monitor files or zipped tar archives from real-world data to analyze. \
            Information about run jobs \
            in a real-world experiment platform."
    )
    parser.add_argument(
        "--data-input-dir",
        type=str,
        help="Directory containing the monitor CSV files from real-world experimentation to analyze. \" \
            Having both the CSV files as well as the archives containing the same files in this directory \
            will lead to double counting! \
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

