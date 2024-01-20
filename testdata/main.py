import heapq
from collections import Counter

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter
from scipy.stats import gaussian_kde


def main():
    with open("traces/trace_mc_server.log", "r", encoding="UTF8") as f:
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

    total = 0
    for line in data:
        total += 16 + 8 + 4 + 4 + len(line[-1])
    print(round(total / 1024 / 1024, ndigits=2), "MiB")

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

    """
    matplotlib.rcParams.update({'font.size': 16})
    fig, ax = plt.subplots(3, figsize=(10, 13), dpi=300)

    density = gaussian_kde(timestamps)
    density.set_bandwidth(0.005)
    xs = np.linspace(tmin, tmax, 500)
    ax[0].plot(xs, density(xs))
    ax[0].set_title('Zeitliche Verteilung der Ereignisse')
    ax[0].set_xlabel("Zeit in s")
    ax[0].set_ylabel("Kerndichteschätzung")

    tdistance = np.log10(distance)

    xs = timestamps[1:]
    ax[1].scatter(xs, distance, c=tdistance, s=0.3, cmap="coolwarm")
    ax[1].set_title('Zeitliche Verteilung der Zeitdifferenz zum vorherigen Ereignis')
    ax[1].set_yscale('log')
    ax[1].set_xlabel("Zeit in s")
    ax[1].set_ylabel("Zeitdifferenz in s")

    density = gaussian_kde(tdistance)
    density.set_bandwidth(0.01)

    xs = np.linspace(min(tdistance), max(tdistance), 500)
    ax[2].plot(xs, density(xs))
    ax[2].xaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0E}'.format(10**y)))
    ax[2].set_title('Verteilung der Zeitdifferenzen')
    ax[2].set_xlabel("Zeitdifferenz in s")
    ax[2].set_ylabel("Kerndichteschätzung")

    fig.tight_layout()
    plt.show()
    # plt.savefig("plot.svg", dpi=10)


def overlaps(x: tuple, y: tuple) -> bool:
    return max(x[0], y[0]) < min(x[0] + x[1], y[0] + y[1])


if __name__ == '__main__':
    main()
