#! /usr/bin/python3

"""
This is a script to create a trapezoid depiction of all simple jobs' lifetimes 
in a WRENCH simulation using the workflow_execution output dump.
The jobs have to consist of three timely seperated phases: 
    - a input-file read phase, 
    - a runtime phase 
    - and a output-file write phase
for this plottingscript to work
"""

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import os.path
import glob
import argparse


def valid_file(param):
    base, ext = os.path.splitext(param)
    if ext.lower() not in ('.csv'):
        raise argparse.ArgumentTypeError('File must have a csv extension')
    return param


parser = argparse.ArgumentParser(
    description="Produce a plot for each file depicting the lifetime of all jobs of the corresponding\
        simulation using a trapezoidal representation. \
        The files containing the simulation dump are CSV files produced by the output method of the simulator.",
    add_help=True
)
parser.add_argument(
    "--njobs",
    type=int,
    default=None,
    help="Optional check for the number of jobs in the passed simulation runs.\
        Will also be present in the name of the output plot."
)
parser.add_argument(
    "--scenario", 
    type=str,
    choices=("copy", "simplifiedstream", "fullstream"),
    required=True,
    help="Choose a scenario, which sets the according plotting label and filename of the plot."
)
parser.add_argument(
    "--suffix",
    type=str,
    help="Optonal suffix to add to the plots' file-names."
)
parser.add_argument(
    "simoutputs",
    nargs='+',
    type=valid_file,
    help="CSV files containing information about the simulated jobs \
        produced by the simulator."
)


args = parser.parse_args()

njobs = args.njobs
scenario = args.scenario
suffix=args.suffix

scenario_plotlabel_dict = {
    "copy": "Input-files copied",
    "simplifiedstream": "Input-files streamed (simpl.)",
    "fullstream": "Block-streaming"
}

machine_color_dict = {
    'sg01': 'red',
    'sg02': 'blue',
    'sg03': 'green'
}

# create a dict of hitrate and corresponding simulation-trace JSON-output-files
outputfiles = args.simoutputs
for outputfile in outputfiles:
    print(outputfile)
    outputfile = os.path.abspath(outputfile)
    assert(os.path.exists(outputfile))

print("Found {0} output-files! Producing {0} trapezoidal plots...".format(len(outputfiles)))
hitrates = [float(outfile.split("_")[-1].strip(".csv").strip("hitrate")) for outfile in outputfiles]

outputfiles_dict = dict(zip(hitrates,outputfiles))
print(outputfiles_dict)


# create a trapez plot for each CSV file corresponding to a hitrate value
fractions = pd.Series([0., float(args.scenario == "copy"), 1., 0.] ) 
for hitrate, outputfile in outputfiles_dict.items():
    with open(outputfile) as f:
        # create a dataframe from CSV 
        df_tmp = pd.read_csv(f, sep=',\t', engine='python')
        if njobs!=None:
            assert(df_tmp.shape[0]==njobs)

    # compute end points and compute streaming share time
    df_tmp["compute.end"] = df_tmp["job.end"] - df_tmp["outfiles.transfertime"]
    df_tmp["transferonly.share"] = df_tmp["compute.end"] - df_tmp["job.computetime"]

    # plot the trapezoid
    fig, ax = plt.subplots()
    ax.set_title(f"hitrate {hitrate}")
    ax.set_xlabel('time / s', loc='right')
    ax.set_xlim([-100.,6100.])
    tmp_label = ""
    for index, row in df_tmp.iterrows():
        if row.filter(items=['machine.name'])[0] != tmp_label:
            ax.plot(
                row.filter(
                    items=["job.start", "transferonly.share", "compute.end", "job.end"]
                ),
                fractions, 
                linewidth=0.1, 
                color=machine_color_dict[row.filter(items=['machine.name'])[0]],
                label = row.filter(items=['machine.name'])[0] 
            )
            tmp_label = row.filter(items=['machine.name'])[0]
        else:
            ax.plot(
                row.filter(
                    items=["job.start", "transferonly.share", "compute.end", "job.end"]
                ),
                fractions, 
                linewidth=0.1, 
                color=machine_color_dict[row.filter(items=['machine.name'])[0]]
            )

    h1, l1 = ax.get_legend_handles_labels()
    legend_dict = dict(zip(l1, h1))
    l1 = list(legend_dict.keys())
    h1 = list(legend_dict.values())
    ax.legend(h1, l1, loc=1)

    plotfilename = "trapezoid"
    if njobs:
        plotfilename += f"{njobs}{scenario}jobs_hitrate{hitrate}"
    if suffix:
        plotfilename += f"{suffix}.pdf"
    else:
        plotfilename += ".pdf"
    fig.savefig(plotfilename)
