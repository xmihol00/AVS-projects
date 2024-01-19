import numpy as np
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser(description='Plot the approximation of the input image.')
parser.add_argument("reference", help="The reference result.")
parser.add_argument("approximated", help="The approximated result.")
args = parser.parse_args()

reference = np.load(args.reference)["d"]
reference = reference == np.max(reference)

approximated = np.load(args.approximated)["d"]
approximated = approximated == np.max(approximated)

optimized_part = reference & ~approximated
blue_zeros = np.zeros((reference.shape[0], reference.shape[1]))


# stack the color channels
rgb = np.stack([approximated, optimized_part, blue_zeros], axis=2)

plt.figure(figsize=(15, 10))
plt.imshow(rgb)

file_name = args.approximated.split("/")[-1].split(".")[0]
plt.savefig(file_name, dpi=300, bbox_inches='tight', pad_inches=0.1)

plt.show()


