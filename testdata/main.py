from collections import Counter

import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import gaussian_kde


def main():
    with open("fifo_trace.txt", "r", encoding="UTF8") as f:
        # fifo write: @<timestamp> <address> <area>: <method>
        data = [x.split(" ", maxsplit=5) for x in f.read().splitlines()]

    timestamps = [int(d[2][1:]) for d in data]
    tmin, tmax = timestamps[0], timestamps[-1]
    print("Timestamp bounds:", tmin, tmax)

    methods = [d[5] for d in data]
    print("Most called methods:", Counter(methods).most_common(25))

    addresses = [(int(d[3], 16), int(d[4][:-1], 10), d[5]) for d in data]
    areas = []
    count = 0
    for add in addresses:
        for i in range(len(areas) - 1, -1, -1):
            old = areas[i]
            if overlaps(add, old):
                count += 1
                print(f"Overlap {count:4d}:", add, "replaced", old)
                del areas[i]

        areas.append(add)

    print("Total overlaps:", count)
    timestamps = [(t - tmin) / (tmax - tmin) for t in timestamps]

    density = gaussian_kde(timestamps)
    density.set_bandwidth(0.005)
    xs = np.linspace(0, 1, 500)
    plt.plot(xs, density(xs))
    plt.show()


def overlaps(x: tuple, y: tuple) -> bool:
    return max(x[0], y[0]) < min(x[0] + x[1], y[0] + y[1])


if __name__ == '__main__':
    main()
