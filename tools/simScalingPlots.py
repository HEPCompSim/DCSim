#! /usr/bin/python3

import pandas as pd
import numpy as np
import scipy.optimize
from matplotlib import pyplot as plt
import os.path
import glob
import argparse


plt.rcParams["figure.autolayout"] = True
pd.set_option('display.max_columns',None)
plt.rcParams['axes.facecolor'] = 'white'
plt.rcParams['axes.spines.left'] = True
plt.rcParams['axes.spines.right'] = True
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


def valid_file(param):
    base, ext = os.path.splitext(param)
    if ext.lower() not in ('.txt', '.dat'):
        raise argparse.ArgumentTypeError('File must have a txt or dat extension')
    return param


scenario_plotlabel_dict = {
    "": "",
    'withdump': "with JSON dump",
    'nodump': "without JSON dump",
    'private': "private improvements",
    "hacky": "final hacky-WRENCH",
    "wrench2": "WRENCH 2.0",
    "SGbatch": "SG-batch",
    "ETPbatch": "ETP-batch",
    "ETPbatchreduced": "reduced ETP-batch"
}


def converttime(df: pd.DataFrame, a: str, b: str):
    return df[a].astype(int)*60 + df[b].astype(int)

def timeexp(x, m, t, b, c):
    return m * np.exp(t * x + c) + b

def timep1(x, m, b):
    return m * x + b

def timep2(x, m, m2, b):
    return m * x + b + m2 * x**2


parser = argparse.ArgumentParser(
    description="Produce a plot showing the runtime and memory scaling of the simulation. \
        If you intend to use this script, make sure that the monitoring files containing the \
        information about the simulation are in the right format. If you produced these by the \
        `simscaling.sh` script, it should work natively.",
    add_help=True
)
parser.add_argument(
    "--scenario", 
    type=str,
    choices=scenario_plotlabel_dict.keys(),
    default="",
    help="Choose a scenario, which sets the according plotting label and filename of the plot."
)
parser.add_argument(
    "monitorfiles",
    nargs='+',
    help="Files containing monitoring information about the simulation run, produced by `ps -aux`.\
        Each file produces a single point in the plot for memory and runtime respectively."
)
parser.add_argument(
    "--extrapolate",
    action='store_true',
    help="Flag to enable exponential, linear, and quadratic extapolations for runtime & memory."
)


args = parser.parse_args()

if args.scenario != "":
    scenario = "_"+args.scenario
else:
    scenario = ""



# Create a data-frame holding all monitoring information
monitorfiles = args.monitorfiles
for mfile in monitorfiles:
    mfile = os.path.abspath(mfile)
    assert(os.path.exists(mfile))
    
print("Found {} monitorfiles".format(str(len(monitorfiles))))

if (all(os.path.exists(f) for f in monitorfiles) and monitorfiles):
    df = pd.concat(
        [
            pd.read_table(
                f,
                delimiter="\s+",
                usecols=[0,1,2,3,4,5,6,7,8,9,10,14],
                names=[
                    "USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND",
                    #"platform option", "Platform file", "njobs option", 
                    "NJobs", #"ninfiles option", "NFilesPerJob",
                    #"insize option", "InFileSize", "siginfiles option", "SigInFileSize", 
                    #"flops option", "Flops", "sigflops option", "SigFlops", "mem option", "Memory",
                    #"outsize option", "OutFileSize", "sigoutsize option", "SigOutFileSize",
                    #"duplications option", "Duplications", "hitrate option", "Hitrate",
                    #"buffersize option", "BufferSize", "xrdblocksize option", "XrdBlockSize", "output option", "OutputName",
                    ],
                )
            for f in monitorfiles
        ],
        ignore_index=True
        )
    print("Simulation monitoring information: \n", df)
else:
    print("Couldn't find any files")
    exit(1)


# postprocess data frame
df['TIME'] = df['TIME'].str.split(":",expand=True).pipe(converttime, 0, 1)
df['RSS'] = df['RSS']/(1024*1024)
runtimesdf = df.loc[df.groupby("NJobs")["TIME"].idxmax()]
memorydf = df.loc[df.groupby("NJobs")["RSS"].idxmax()]
print("Filtered data:\n", runtimesdf)


# Visualize the monitoring information
fig, ax = plt.subplots()
ax.set_title(scenario_plotlabel_dict[scenario])

# ax.set_xscale('log')
ax.set_xlabel('Number of Active Job Slots', loc='right')
ax.set_ylabel('Time / min', color='cornflowerblue')
ax.set_xlim([0,runtimesdf['NJobs'].iloc[-1]*1.05])
# ax.set_ylim([0,400])

ax.plot(runtimesdf['NJobs'], runtimesdf['TIME']/60, linestyle='dotted', color='cornflowerblue')
ax.scatter(runtimesdf['NJobs'], runtimesdf['TIME']/60, color='cornflowerblue', marker='x', label='runtime')
# ax.grid(axis="y", linestyle = 'dotted', which='major')

secax = ax.twinx()
secax.plot(memorydf['NJobs'], memorydf['RSS'],linestyle='dotted', color='orange')
secax.scatter(memorydf['NJobs'], memorydf['RSS'], color='orange', marker='^', label='memory')
# secax.xaxis.set_minor_locator(AutoMinorLocator())
secax.set_ylabel('Memory / GiB', color='orange')
# secax.set_ylim([0,12])

h1, l1 = ax.get_legend_handles_labels()
h2, l2 = secax.get_legend_handles_labels()
ax.legend(h1+h2, l1+l2, loc=2)

njobs = np.linspace(runtimesdf['NJobs'].iloc[0], runtimesdf['NJobs'].iloc[-1], 1000)


if args.extrapolate:
    print("RUNTIME EXTRAPOLATIONS for 100k job slots:")
    start_params = (0.0, 0.0, 0.0, 0.0)
    params, cv = scipy.optimize.curve_fit(timeexp, runtimesdf['NJobs'], runtimesdf['TIME']/60, start_params)
    m, t, b, c = params
    if np.all(np.isfinite(cv)):
        ax.plot(njobs, timeexp(njobs, m, t, b, c), linestyle='-', color='blue', label='runtime extrapolation')
        print(f"\tExponential: {timeexp(100000, m, t, b, c)} min = {timeexp(100000, m, t, b, c)/60.} h = {timeexp(100000, m, t, b, c)/60./24.} d")
        print(f"\tfunction: m * exp(t * x + c) + b;    m, t, b, c = {params}")
    else:
        print(f"\tExponential fit failed. Please check initial parameters.")
    print("")

    start_params = (0.01, 0.01)
    params, cv = scipy.optimize.curve_fit(timep1, runtimesdf['NJobs'], runtimesdf['TIME']/60, start_params)
    m, b = params
    if np.all(np.isfinite(cv)):
        ax.plot(njobs, timep1(njobs, m, b), linestyle='-', color='green', label='runtime extrapolation')
        print(f"\tLinear: {timep1(100000, m, b)} min = {timep1(100000, m, b)/60.} h = {timep1(100000, m, b)/60./24.} d")
        print(f"\tfunction: m * x + b;    m, b = {params}")
    else:
        print(f"\tLinear fit failed. Please check initial parameters.")
    print("")

    start_params = (0.01, 0.01, 0.01)
    params, cv = scipy.optimize.curve_fit(timep2, runtimesdf['NJobs'], runtimesdf['TIME']/60, start_params)
    m, m2, b = params
    if np.all(np.isfinite(cv)):
        ax.plot(njobs, timep2(njobs, m, m2, b), linestyle='-', color='red', label='runtime extrapolation')
        print(f"\tQuadratic: {timep2(100000, m, m2, b)} min = {timep2(100000, m, m2, b)/60.} h = {timep2(100000, m, m2, b)/60./24.} d")
        print(f"\tfunction: m * x + m2 * x**2 + b;    m, m2, b = {params}")
    else:
        print(f"\tQuadratic fit failed. Please check initial parameters.")
    print("")

    print("MEMORY EXTRAPOLATION for 100k jobs:")
    start_params = (0.0, 0.0, 0.0, 0.0)
    params, cv = scipy.optimize.curve_fit(timeexp, memorydf['NJobs'], memorydf['RSS'], start_params)
    m, t, b, c = params
    if np.all(np.isfinite(cv)):
        ax.plot(njobs, timeexp(njobs, m, t, b, c), linestyle='--', color='blue', label='runtime extrapolation')
        print(f"\tExponential: {timeexp(100000, m, t, b, c)} GB")
        print(f"\tfunction: m * exp(t * x + c) + b;    m, t, b, c = {params}")
    else:
        print(f"\tExponential fit failed. Please check initial parameters.")
    print("")
    start_params = (0.0, 0.0)
    params, cv = scipy.optimize.curve_fit(timep1, memorydf['NJobs'], memorydf['RSS'], start_params)
    m, b = params
    if np.all(np.isfinite(cv)):
        ax.plot(njobs, timep1(njobs, m, b), linestyle='--', color='green', label='runtime extrapolation')
        print(f"\tLinear: {timep1(100000, m, b)} GB")
        print(f"\tfunction: m * x + b;    m, b = {params}")
    else:
        print(f"\tLinear fit failed. Please check initial parameters.")
    print("")

    start_params = (0.0, 0.0, 0.0)
    params, cv = scipy.optimize.curve_fit(timep2, memorydf['NJobs'], memorydf['RSS'], start_params)
    m, m2, b = params
    if np.all(np.isfinite(cv)):
        ax.plot(njobs, timep2(njobs, m, m2, b), linestyle='--', color='red', label='runtime extrapolation')
        print(f"\tQuadratic: {timep2(100000, m, m2, b)} GB")
        print(f"\tfunction: m * x + m2 * x**2 + b;    m, m2, b = {params}")
    else:
        print(f"\tQuadratic fit failed. Please check initial parameters.")
    print("")

fig.savefig("scalingtest"+ scenario +".pdf")
fig.savefig("scalingtest"+ scenario +".png")

# plt.show()
