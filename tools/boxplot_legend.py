

import numpy as np
from matplotlib import pyplot as plt
import os

plt.rcParams['figure.autolayout'] = True
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

fig = plt.figure(f"boxplot_legend", figsize=(3,2))

ax_in = fig.add_subplot(1,1,1)

ax_in.boxplot(np.linspace(0.,4.,100), medianprops = dict(color="black"))
ax_in.axis("off")
ax_in.annotate("Q3$+ 1.5$IQR", xy=(1.1,4), xytext=(1.2,3.8), arrowprops=dict(arrowstyle="-", color="black",), ha="left")
ax_in.annotate("Q3$= 75$ percentile", xy=(1.1,3), xytext=(1.2,2.8), arrowprops=dict(arrowstyle="-", color="black",), ha="left")
ax_in.annotate("median", xy=(1.1,2), xytext=(1.2,1.8), arrowprops=dict(arrowstyle="-", color="black",))
ax_in.annotate(f"Q1$= 25$ percentile", xy=(1.1,1), xytext=(1.2,0.8), arrowprops=dict(arrowstyle="-", color="black",), ha="left")
ax_in.annotate(f"Q1$- 1.5$IQR", xy=(1.1,0), xytext=(1.2,-0.2), arrowprops=dict(arrowstyle="-", color="black",), ha="left")
ax_in.annotate('', xy=(0.85,0.898), xytext=(0.85, 3.079), arrowprops={'arrowstyle': '<->'})
ax_in.annotate('IQR', xy=(0.8, 1.8), xytext=(0.8, 1.8), ha="right")
ax_in.set_yticklabels([])
ax_in.set_xticklabels([])

fig.savefig(os.path.join(os.getcwd(), f"{fig.get_label()}.pdf"))
fig.savefig(os.path.join(os.getcwd(), f"{fig.get_label()}.png"))
plt.close()
