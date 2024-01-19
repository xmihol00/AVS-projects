import pandas as pd
import argparse

parser = argparse.ArgumentParser(description="Compare two datalogs")
parser.add_argument("file1", help="First datalog")
parser.add_argument("file2", help="Second datalog")
args = parser.parse_args()

df1 = pd.read_csv(args.file1, sep=";")
df2 = pd.read_csv(args.file2, sep=";")

df1 = df1.loc[df1["CALCULATOR"] == "LineMandelCalculator"]
df2 = df2.loc[df2["CALCULATOR"] == "LineMandelCalculator"]

# compare times
t1 = df1["TIME"].values
t2 = df2["TIME"].values

diff = t1 - t2
first = (diff > 0).sum()
second = (diff < 0).sum()

if first > second:
    print("Second datalog is faster")
elif first < second:
    print("First datalog is faster")
else:
    print("Datalogs are equally fast")
print(diff, diff.sum())
