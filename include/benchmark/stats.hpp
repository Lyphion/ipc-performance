#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace ipc::benchmark {

struct Statistics {
    const double minimum;
    const double first_quartile;
    const double median;
    const double third_quartile;
    const double maximum;
    const double average;
    const double variance;
    const double standard_deviation;

    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    static Statistics compute(const std::vector<T> &data);
};

template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
static inline double quantile(const std::vector<T> &data, double quantile) {
    const double poi = (1 - quantile) * -0.5 + quantile * (data.size() - 0.5);

    const auto left = std::max(int64_t(std::floor(poi)), int64_t(0));
    const auto right = std::min(int64_t(std::ceil(poi)), int64_t(data.size() - 1));

    const auto data_left = data[left];
    const auto data_right = data[right];

    const auto tmp = poi - static_cast<double>(left);
    return (1 - tmp) * data_left + tmp * data_right;
}

template<typename T, typename>
Statistics Statistics::compute(const std::vector<T> &data) {
    auto tmp = data;
    std::sort(tmp.begin(), tmp.end());

    const auto sum = std::reduce(tmp.begin(), tmp.end());
    const auto avg = static_cast<double>(sum) / static_cast<double>(tmp.size());

    const auto add_square = [avg](double sum, int i) {
        const auto d = i - avg;
        return sum + d * d;
    };

    const auto total = std::accumulate(tmp.begin(), tmp.end(), 0.0, add_square);
    const auto v = total / static_cast<double>(tmp.size() - 1);

    return Statistics{
            .minimum = static_cast<double>(tmp.front()),
            .first_quartile = quantile(tmp, 0.25),
            .median = quantile(tmp, 0.5),
            .third_quartile = quantile(tmp, 0.75),
            .maximum = static_cast<double>(tmp.back()),
            .average = avg,
            .variance = v,
            .standard_deviation = std::sqrt(v)
    };
}

}
