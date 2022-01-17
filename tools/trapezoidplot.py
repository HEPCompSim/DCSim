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
import json


njobs = 60
scenario = "copy"
suffix="test"

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
output_dir = os.path.join(os.path.dirname(__file__), "..", "sgbatch", "tmp", "outputs")
print(f"Searching for simulation traces in {output_dir}")
outputfiles = glob.glob(os.path.abspath(os.path.join(output_dir, f"hitratescaling_{scenario}_{njobs}jobs_hitrate*.csv")))

print("Found {} output-files!".format(len(outputfiles)))
hitrates = [float(outfile.split("_")[-1].strip(".csv").strip("hitrate")) for outfile in outputfiles]

outputfiles_dict = dict(zip(hitrates,outputfiles))
print(outputfiles_dict)


# create a trapez plot for each JSON file corresponding to a hitrate value
fractions = pd.Series([0., 0.5, 1., 0.])
for hitrate, outputfile in outputfiles_dict.items():
    with open(outputfile) as f:
        # create a dataframe from JSON 
        df_tmp = pd.read_csv(f, sep=',\t', engine='python')

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

    fig.savefig(f"trapezoid_{njobs}{scenario}jobs_hitrate{hitrate}.pdf")