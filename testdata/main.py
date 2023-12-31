import heapq
from collections import Counter

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter
from scipy.stats import gaussian_kde


def main():
    with open("fifo_trace.txt", "r", encoding="UTF8") as f:
        # fifo write: @<timestamp> <address> <area>: <method>
        data = [x.split(" ", maxsplit=5) for x in f.read().splitlines()]

    timestamps = [int(d[2][1:]) for d in data]
    tmin, tmax = timestamps[0], timestamps[-1]
    print("Timestamp bounds:", tmin, tmax)
    timestamps = [(t - tmin) / 1e9 for t in timestamps]
    tmin, tmax = timestamps[0], timestamps[-1]

    methods = [d[5] for d in data]
    print("Most called methods:", Counter(methods).most_common(25))

    distance = [(timestamps[i + 1] - timestamps[i]) for i in range(len(timestamps) - 1)]
    print("10 smallest distances:", [f"{t * 1e6:.2F}us" for t in heapq.nsmallest(10, distance)])
    print("Distances under 15us:", len([d for d in distance if d <= 15e-6]))
    print("Distances between 15 and 50us:", len([d for d in distance if 50e-6 >= d > 15e-6]))
    print("Distances between 50 and 200us:", len([d for d in distance if 200e-6 >= d > 50e-6]))
    print("Distances greater than 200us:", len([d for d in distance if d > 200e-6]))
    print("Distances greater than 1ms:", len([d for d in distance if d > 1e-3]))

    """
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
    """

    # timestamps = [(t - tmin) / (tmax - tmin) for t in timestamps]

    fig, ax = plt.subplots(3, figsize=(10, 10), dpi=300)

    density = gaussian_kde(timestamps)
    density.set_bandwidth(0.01)
    xs = np.linspace(tmin, tmax, 500)
    ax[0].plot(xs, density(xs))
    ax[0].set_title('Event occurrences')

    tdistance = np.log10(distance)

    xs = timestamps[1:]
    ax[1].scatter(xs, distance, c=tdistance, s=0.3, cmap="coolwarm")
    ax[1].set_title('Time between events')
    ax[1].set_yscale('log')

    density = gaussian_kde(tdistance)
    density.set_bandwidth(0.01)

    xs = np.linspace(min(tdistance), max(tdistance), 500)
    ax[2].plot(xs, density(xs))
    ax[2].xaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0E}'.format(10**y)))
    ax[2].set_title('Distribution of time between events')

    fig.tight_layout()
    plt.show()
    # plt.savefig("plot.svg", dpi=10)


def overlaps(x: tuple, y: tuple) -> bool:
    return max(x[0], y[0]) < min(x[0] + x[1], y[0] + y[1])


if __name__ == '__main__':
    main()
