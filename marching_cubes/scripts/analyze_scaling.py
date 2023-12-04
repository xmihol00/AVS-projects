import re
import os
import pandas as pd
import glob
import matplotlib.pyplot as plt

regex = r"(?:Elapsed Time:\s+(\d+) ms)"

fig_title = "Slabé škálování při různých velikostech grid"
directory = "../avs-proj02/grid_scaling"
threads = [2, 4, 8, 16, 32]
threads_grids = ["vlákna: 2, grid: 16", "vlákna: 4, grid: 32", "vlákna: 8, grid: 64", "vlákna: 16, grid: 128", "vlákna: 32, grid: 256"]
xlabel = "Počet vláken, velikost grid"

fig_title = "Slabé škálování při různých velikostech parametrického pole"
directory = "../avs-proj02/field_scaling"
threads = [4, 8, 16, 32]
threads_grids = ["vlákna: 4, field: 4", "vlákna: 8, field: 3", "vlákna: 16, field: 2", "vlákna: 32, field: 1"]
xlabel = "Počet vláken, velikost parametrického pole"

builders = ["loop", "tree"]
builder_dict = { builder: {} for builder in builders }
divisor = 1

for thread in threads:
    for builder in builders:
        for file in glob.glob(os.path.join(directory, f"{builder}_{thread}*.txt")):
            with open(file, 'r') as f:
                content = f.read()
        
            matches = re.findall(regex, content, re.MULTILINE)
            values = [int(match) for match in matches]
            mean = sum(values) / len(values)
            builder_dict[builder][thread] = mean / divisor

plt.figure(figsize=(18, 9))
plt.title(fig_title)
plt.xlabel(xlabel)
plt.ylabel("Doba běhu [ms]")  
plt.plot(threads_grids, builder_dict["loop"].values(), label="Loop", marker="o")
plt.plot(threads_grids, builder_dict["tree"].values(), label="Octree", marker="o")
plt.legend()
plt.tight_layout()
plt.savefig(f"{directory}/weak_scaling.png", dpi=500)
plt.show()

# save to csv
df = pd.DataFrame.from_dict(builder_dict)
df.index = threads_grids
df.to_csv(f"{directory}/weak_scaling.csv")
print(df)
