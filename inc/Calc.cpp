#include "Calc.h"
#include <cmath>
#include <limits>

float Calc::sumSquares(const std::vector<float>& v) const {
    double acc = 0.0;
    for (float x : v) {
        acc += static_cast<double>(x) * static_cast<double>(x);
    }

    constexpr double FLT_MAX_D = std::numeric_limits<float>::max();

    if (std::isnan(acc))
        return std::numeric_limits<float>::quiet_NaN();

    if (acc > FLT_MAX_D)
        return std::numeric_limits<float>::infinity();
    if (acc < -FLT_MAX_D)
        return -std::numeric_limits<float>::infinity();

    return static_cast<float>(acc);
}
