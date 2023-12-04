import re
import os
import pandas as pd
import glob
import matplotlib.pyplot as plt

regex = r"(?:Input Field File:\s+../data/(.*?).pts)|(?:Elapsed Time:\s+(\d+) ms)"

version = "default"

if version == "optimized":
    directories = ["../avs-proj02/dynamic_schedules", "../avs-proj02/static_schedules", "../avs-proj02/guided_schedules"]
elif version == "default":
    directories = ["../avs-proj02/def_dynamic_sched", "../avs-proj02/def_static_sched", "../avs-proj02/def_guided_sched"]

sched_dict = { }
scheduler_names = set()
maxes = {}

for directory in directories:
    for file in glob.glob(os.path.join(directory, "*.txt")):
        res_dict = {}

        with open(file, 'r') as f:
            content = f.read()
        
        matches = re.findall(regex, content, re.MULTILINE)
        for i in range(0, len(matches), 2):
            if matches[i][0] not in res_dict:
                res_dict[matches[i][0]] = []
            res_dict[matches[i][0]].append(int(matches[i+1][1]))
        
        df_mean = pd.DataFrame.from_dict(res_dict).mean(axis=0)

        name = "_".join(file.split('/')[-1].split('.')[0].split('_')[0:2])
        if name not in sched_dict:
            sched_dict[name] = {}

        for row in df_mean.items():           
            if "64." in file:
                name_suffix = row[0] + "_64"
            elif "128." in file:
                name_suffix = row[0] + "_128"
            name_suffix = name_suffix.replace("bun_zipper", "BZ").replace("dragon_vrip", "DV")
            if name_suffix not in maxes:
                maxes[name_suffix] = 0
            
            if row[1] > maxes[name_suffix]:
                maxes[name_suffix] = row[1]

            scheduler_names.add(name_suffix)
            sched_dict[name][name_suffix] = row[1]

for file_dict in sched_dict.values():
    for file_name in scheduler_names:
        file_dict[file_name] = file_dict[file_name] / maxes[file_name]

for key, value in sched_dict.items():
    sched_dict[key] = sum(value.values())

max_sched = max(sched_dict.values())
for key, value in sched_dict.items():
    sched_dict[key] = value / max_sched
min_sched = min(sched_dict.values())

plt.figure(figsize=(18, 9))
plt.title("Relativní průměrné délky běhů různých plánovačů")
for key, value in sched_dict.items():
    plt.bar(key, value)
    if value <= min_sched + 0.00001:
        plt.annotate(f"{value:.4f}", (key, value), ha='center', va='bottom', weight='bold')
    else:
        plt.annotate(f"{value:.4f}", (key, value), ha='center', va='bottom')
plt.ylabel("Relativní průměrná délka běhu")
plt.xlabel("Plánovač")
plt.tight_layout()
plt.savefig(f"../avs-proj02/relative_averaged_schedulers_{version}.png", dpi=500)
plt.show()

result_df = pd.DataFrame.from_dict(sched_dict, orient='index', columns=['Relativní průměrná délka běhu'])
sorted_df = result_df.sort_values(by=['Relativní průměrná délka běhu'])
sorted_df.to_csv(f"../avs-proj02/relative_averaged_schedulers_{version}.csv")
print(sorted_df)
