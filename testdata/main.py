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

    methods = [d[5] for d in data]
    print("Most called methods:", Counter(methods).most_common(25))

    distance = [(timestamps[i + 1] - timestamps[i]) / 1000 for i in range(len(timestamps) - 1)]
    print("10 smallest distances:", heapq.nsmallest(10, distance))
    print("Distances under 15us:", len([d for d in distance if d <= 15]))
    print("Distances between 15 and 50us:", len([d for d in distance if 50 >= d > 15]))
    print("Distances between 50 and 200us:", len([d for d in distance if 200 >= d > 50]))
    print("Distances greater than 200us:", len([d for d in distance if d > 200]))

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

    timestamps = [(t - tmin) / (tmax - tmin) for t in timestamps]

    fig, ax = plt.subplots(2)

    density = gaussian_kde(timestamps)
    density.set_bandwidth(0.01)
    xs = np.linspace(0, 1, 500)
    ax[0].plot(xs, density(xs))
    ax[0].set_title('Event occurrences')

    distance = np.log10(distance)
    density = gaussian_kde(distance)
    density.set_bandwidth(0.01)

    xs = np.linspace(min(distance), max(distance), 500)
    ax[1].plot(xs, density(xs))
    ax[1].xaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0E}'.format(10**y)))
    ax[1].set_title('Time delta between events')

    fig.tight_layout()
    plt.show()


def overlaps(x: tuple, y: tuple) -> bool:
    return max(x[0], y[0]) < min(x[0] + x[1], y[0] + y[1])


if __name__ == '__main__':
    main()
