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

machine_color_dict = {
    'sg01': 'red',
    'sg02': 'blue',
    'sg03': 'green'
}

# create a dict of hitrate and corresponding simulation-trace JSON-output-files
outputfiles = glob.glob(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "sgbatch", "tmp", "outputs", f"unified*{njobs}jobs.json")))
hitrates = [float(outfile.split("_")[1].strip("h")) for outfile in outputfiles]
outputfiles_dict = dict(zip(hitrates,outputfiles))

# create a trapez plot for each JSON file corresponding to a hitrate value
fractions = pd.Series([0., 1., 1., 0.])
for hitrate, outputfile in outputfiles_dict.items():
    with open(outputfile) as f:
        # create a dataframe from JSON 
        df_tmp = pd.json_normalize(json.load(f)['workflow_execution']['tasks'])

    # filter dataframe for necessary information
    df_tmp = df_tmp.filter(
        items=["whole_task.start", "whole_task.end", "compute.start", "compute.end", "execution_host.hostname"]
    )

    # plot the trapezoid
    fig, ax = plt.subplots()
    ax.set_title("hitrate {}".format(hitrate))
    ax.set_xlabel('time / s', loc='right')
    ax.set_xlim([-100.,6100.])
    tmp_label = ""
    for index, row in df_tmp.iterrows():
        if row.filter(items=['execution_host.hostname'])[0] != tmp_label:
            ax.plot(
                row.filter(
                    items=["whole_task.start", "compute.start", "compute.end", "whole_task.end"]
                ),
                fractions, 
                linewidth=0.1, 
                color=machine_color_dict[row.filter(items=['execution_host.hostname'])[0]],
                label = row.filter(items=['execution_host.hostname'])[0] 
            )
            tmp_label = row.filter(items=['execution_host.hostname'])[0]
        else:
            ax.plot(
                row.filter(
                    items=["whole_task.start", "compute.start", "compute.end", "whole_task.end"]
                ),
                fractions, 
                linewidth=0.1, 
                color=machine_color_dict[row.filter(items=['execution_host.hostname'])[0]]
            )

    h1, l1 = ax.get_legend_handles_labels()
    legend_dict = dict(zip(l1, h1))
    l1 = list(legend_dict.keys())
    h1 = list(legend_dict.values())
    ax.legend(h1, l1, loc=1)

    fig.savefig("trapezoid_{}jobs_hitrate{}.pdf".format(njobs, hitrate))