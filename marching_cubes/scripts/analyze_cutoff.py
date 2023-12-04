import re
import os
import pandas as pd
import glob
import matplotlib.pyplot as plt

regex = r"(?:Input Field File:\s+../data/(.*?).pts)|(?:Elapsed Time:\s+(\d+) ms)"

fig_title_1 = "Relativní délky běhů různých cut-off Octree algoritmu"
fig_title_2 = "Relativní součty normalizovaných délek různých cut-off Octree algoritmu"
directory = "../avs-proj02/cut_off_36"

mean_64_dict = {}
mean_128_dict = {}
mean_256_dict = {}
mean_512_dict = {}

for file in glob.glob(os.path.join(directory, "*.txt")):
    res_dict = {}

    with open(file, 'r') as f:
        content = f.read()
    
    matches = re.findall(regex, content, re.MULTILINE)
    for i in range(0, len(matches), 2):
        if matches[i][0] not in res_dict:
            res_dict[matches[i][0]] = []
        print(matches[i+1][1], file, matches[i][0])
        res_dict[matches[i][0]].append(int(matches[i+1][1]))
    
    df = pd.DataFrame.from_dict(res_dict)
    df_mean = df.mean(axis=0)

    file_name = file.split('/')[-1].split('.')[0]
    for row in df_mean.items():

        if "64." in file:
            if row[0] not in mean_64_dict:
                mean_64_dict[row[0]] = []
            mean_64_dict[row[0]].append((file_name, row[1]))
        
        if "128." in file:
            if row[0] not in mean_128_dict:
                mean_128_dict[row[0]] = []
            mean_128_dict[row[0]].append((file_name, row[1]))
        
        if "256." in file:
            if row[0] not in mean_256_dict:
                mean_256_dict[row[0]] = []
            mean_256_dict[row[0]].append((file_name, row[1]))
        
        if "512." in file:
            if row[0] not in mean_512_dict:
                mean_512_dict[row[0]] = []
            mean_512_dict[row[0]].append((file_name, row[1]))
    
for key in mean_64_dict:
    mean_64_dict[key].sort(key=lambda x: x[1])
for key in mean_128_dict:
    mean_128_dict[key].sort(key=lambda x: x[1])
for key in mean_256_dict:
    mean_256_dict[key].sort(key=lambda x: x[1])
for key in mean_512_dict:
    mean_512_dict[key].sort(key=lambda x: x[1])

dicts = [(mean_64_dict, "64"), (mean_128_dict, "128"), (mean_256_dict, "256"), (mean_512_dict, "512")]
for dict, cutoff in dicts:
    relative_dict = {}
    for key in dict:
        _, max = dict[key][-1]

        for name, value in dict[key]:
            name += "X"
            name = name.replace(f"_{cutoff}X", "")
            if name not in relative_dict:
                relative_dict[name] = ([], [])
            relative_dict[name][0].append(key.replace("bun_zipper", "BZ").replace("dragon_vrip", "DV") + f"_{cutoff}")
            relative_dict[name][1].append(value / max)
    
    plt.figure(figsize=(18, 9))
    plt.title(fig_title_1 + f" (grid: {cutoff})")
    for key in relative_dict:
        plt.plot(relative_dict[key][0], relative_dict[key][1], label=key)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{directory}/relative_plot_{cutoff}.png", dpi=500)
    #plt.show()

    max = 0
    min = 99999999999
    for key in relative_dict:
        relative_dict[key] = sum(relative_dict[key][1])
        if relative_dict[key] > max:
            max = relative_dict[key]
        if relative_dict[key] < min:
            min = relative_dict[key]
    min /= max

    plt.figure(figsize=(18, 9))
    plt.title(fig_title_2 + f" (grid: {cutoff})")
    for key in relative_dict:
        relative_dict[key] /= max
        plt.bar(key, relative_dict[key])
        if relative_dict[key] <= min + 0.0000001:
            plt.annotate(f"{relative_dict[key]:.4f}", (key, relative_dict[key]), ha='center', va='bottom', weight='bold')
        else:
            plt.annotate(f"{relative_dict[key]:.4f}", (key, relative_dict[key]), ha='center', va='bottom')
    plt.tight_layout()
    plt.savefig(f"{directory}/relative_bar_{cutoff}.png", dpi=500)
    #plt.show()

    df_sorted = pd.DataFrame.from_dict(relative_dict, orient='index', columns=[f'Relativní průměrná délka běhu - grid {cutoff}']).sort_values(by=[f'Relativní průměrná délka běhu - grid {cutoff}'])
    df_sorted.to_csv(f"{directory}/relative_table_{cutoff}.csv")
    print(df_sorted)

relative_dict = {}
#for key in mean_32_dict:
#    _, max = mean_32_dict[key][-1]
#
#    for name, value in mean_32_dict[key]:
#        name += "X"
#        name = name.replace("_32X", "")
#        if name not in relative_dict:
#            relative_dict[name] = ([], [])
#        relative_dict[name][0].append(key.replace("bun_zipper", "BZ").replace("dragon_vrip", "DV") + "_32")
#        relative_dict[name][1].append(value / max)

plt.cla()
plt.clf()

for key in mean_64_dict:
    _, max = mean_64_dict[key][-1]

    for name, value in mean_64_dict[key]:
        name += "X"
        name = name.replace("_64X", "")
        if name not in relative_dict:
            relative_dict[name] = ([], [])
        relative_dict[name][0].append(key.replace("bun_zipper", "BZ").replace("dragon_vrip", "DV") + "_64")
        relative_dict[name][1].append(value / max)

for key in mean_128_dict:
    _, max = mean_128_dict[key][-1]

    for name, value in mean_128_dict[key]:
        name += "X"
        name = name.replace("_128X", "")
        relative_dict[name][0].append(key.replace("bun_zipper", "BZ").replace("dragon_vrip", "DV") + "_128")
        relative_dict[name][1].append(value / max)

for key in mean_256_dict:
    _, max = mean_256_dict[key][-1]

    for name, value in mean_256_dict[key]:
        name += "X"
        name = name.replace("_256X", "")
        relative_dict[name][0].append(key.replace("bun_zipper", "BZ").replace("dragon_vrip", "DV") + "_256")
        relative_dict[name][1].append(value / max)

for key in mean_512_dict:
    _, max = mean_512_dict[key][-1]

    for name, value in mean_512_dict[key]:
        name += "X"
        name = name.replace("_512X", "")
        relative_dict[name][0].append(key.replace("bun_zipper", "BZ").replace("dragon_vrip", "DV") + "_512")
        relative_dict[name][1].append(value / max)

plt.figure(figsize=(18, 9))
plt.title(fig_title_1)
for key in relative_dict:
    plt.plot(relative_dict[key][0], relative_dict[key][1], label=key)
plt.legend()
plt.tight_layout()
plt.savefig(f"{directory}/relative_plot.png", dpi=500)
plt.show()

max = 0
min = 99999999999
for key in relative_dict:
    relative_dict[key] = sum(relative_dict[key][1])
    if relative_dict[key] > max:
        max = relative_dict[key]
    if relative_dict[key] < min:
        min = relative_dict[key]
min /= max

plt.figure(figsize=(18, 9))
plt.title(fig_title_2)
for key in relative_dict:
    relative_dict[key] /= max
    plt.bar(key, relative_dict[key])
    if relative_dict[key] <= min + 0.00001:
        plt.annotate(f"{relative_dict[key]:.4f}", (key, relative_dict[key]), ha='center', va='bottom', weight='bold')
    else:
        plt.annotate(f"{relative_dict[key]:.4f}", (key, relative_dict[key]), ha='center', va='bottom')
plt.tight_layout()
plt.savefig(f"{directory}/relative_bar.png", dpi=500)
plt.show()

df_sorted = pd.DataFrame.from_dict(relative_dict, orient='index', columns=['Relativní průměrná délka běhu']).sort_values(by=['Relativní průměrná délka běhu'])
df_sorted.to_csv(f"{directory}/relative_table.csv")
print(df_sorted)
