#! /usr/bin/python3

from platform import platform
import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
from matplotlib.font_manager import FontProperties
import os.path
import argparse

# plt.rcParams["figure.figsize"] = [4., 3.]
plt.rcParams["figure.autolayout"] = True

def validFileLabelTuple(param: str):
    """
    Helper function, which transforms and checks for the right structure of the given arguments
    
    param param: the argument to check

    return: tuple[str, str]
    """
    # check for valid structure of the argument
    try:
        filepath, label = map(str, param.split(','))
    except ValueError:
        filepath = param.split(',')
        if len(filepath) > 1:
            exit(f"Argument {param} has too many values")
        elif len(filepath) < 1:
            exit(f"Argument {param} has too few values")
        elif len(filepath) == 1:
            filepath = param
        label = ""
    except Exception as e: 
        print(e)
        raise argparse.ArgumentError(f"Unsupported argument {param}")

    # ensure the right file extension
    ext = os.path.splitext(filepath)[1]
    if ext.lower() not in ('.csv'):
        raise argparse.ArgumentTypeError(f"Invalid file type: {param} must be a csv file")
    
    # ensure that file exists
    if not os.path.exists(filepath):
        raise argparse.ArgumentTypeError(f"Invalid file {filepath} does not exist")
    
    return (str(filepath), label)
    


parser = argparse.ArgumentParser(
    description="Produce plots showing the performance of the simulated system. \
        It can use several files, or tuples of file and label, one for each simulation run to be compared. \
        The files containing the simulation dump are CSV files produced by the output method of the simulator.",
    add_help=True
)
parser.add_argument(
    "--logscale",
    action="store_true",
    help="Plot with logarithmic x-axis"
)
parser.add_argument(
    "--title",
    type=str,
    default="DCSim",
    help="Plot title hinting on the simulated scenario"
)
parser.add_argument(
    "--suffix",
    type=str,
    help="Optonal suffix to add to the file-name of the plot."
)
parser.add_argument(
    "simoutputs",
    nargs='+',
    type=validFileLabelTuple,
    help="CSV files, or tuples of file and label, containing information \
        about the simulated jobs produced by the simulator."
)


args = parser.parse_args()

title = args.title
suffix = args.suffix
file_label_pairs = args.simoutputs

file_label_pairs = dict(
    file_label_pairs
)


event_fig, event_ax = plt.subplots()
event_ax.set_xlabel('time / s', loc='right')
if args.logscale:
    event_ax.set_xscale('log')

for file, label in file_label_pairs.items():
    with open(file) as f:
        df = pd.read_csv(f, sep=",\s", engine='python')
    
    starts = event_ax.eventplot(
        positions=df['job.start'].to_numpy(),
        orientation='horizontal',
        lineoffsets=label,
        linewidths=0.1,
        linelengths=0.75,
        colors='black',
        label="start"
    )
    ends = event_ax.eventplot(
        positions=df['job.end'].to_numpy(),
        orientation='horizontal',
        lineoffsets=label,
        linewidths=0.1,
        linelengths=0.75,
        colors='black',
        label="end"
    )


    machines = df["machine.name"].unique()


    efficiency_fig, efficiency_ax = plt.subplots()
    efficiency_ax.set_xlabel("eff. / %", loc='right')
    efficiency_ax.set_ylabel("jobs", loc='top')
    efficiency_ax.set_yscale('log')

    walltime_fig, walltime_ax = plt.subplots()
    walltime_ax.set_xlabel("walltime / s", loc='right')
    walltime_ax.set_ylabel("jobs", loc='top')
    walltime_ax.set_yscale('log')

    machine_efficiencies = {}
    machine_walltimes = {}
    for i,machine in enumerate(machines):
        df_masked = df[df["machine.name"]==machine]

        machine_efficiency = df_masked["job.computetime"]/(df_masked["job.end"]-df_masked["job.start"])/100
        machine_efficiencies[machine]=machine_efficiency

        machine_walltime = (df_masked["job.end"]-df_masked["job.start"])
        machine_walltimes[machine]=machine_walltime

    machine_efficiencies_list = sorted(machine_efficiencies.items(),key=lambda x: x[1].size)
    machine_efficiencies = dict(machine_efficiencies_list)
    machine_walltimes_list = sorted(machine_walltimes.items(), key=lambda x: x[1].size)
    machine_walltimes = dict(machine_walltimes_list)

    efficiency_ax.hist(
        list(machine_efficiencies.values()),
        bins=100, range=(0.,100.),
        stacked=True,
        label=list(machine_efficiencies.keys())
    )

    walltime_ax.hist(
        list(machine_walltimes.values()),
        bins=100,
        stacked=True,
        label=list(machine_walltimes.keys())
    )


    efficiency_ax.legend()
    efficiency_ax.set_title(title+" "+label, loc='left', fontsize=14, fontweight='bold')

    efficiency_fig.savefig(f"efficiency_{label}_{suffix}.png")
    efficiency_fig.savefig(f"efficiency_{label}_{suffix}.pdf")


    walltime_ax.legend()
    walltime_ax.set_title(title+" "+label, loc='left', fontsize=14, fontweight='bold')

    walltime_fig.savefig(f"walltime_{label}_{suffix}.png")
    walltime_fig.savefig(f"walltime_{label}_{suffix}.pdf")


event_ax.legend(
    handles = (starts[0], ends[0]),
    labels = ("start", "end")
)
event_ax.set_title(title, loc='left', fontsize=14, fontweight='bold')

event_fig.savefig(f"jobevents_{suffix}.png")
event_fig.savefig(f"jobevents_{suffix}.pdf")



plt.show()

