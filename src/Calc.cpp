#include "Calc.h"
#include <cmath>
#include <limits>

float Calc::sumSquares(const std::vector<float>& v) const {
    double acc = 0.0;
    for (float x : v) {
        double xx = static_cast<double>(x) * x;
        acc += xx;
    }
    float result = static_cast<float>(acc);
    if (std::isinf(acc) || std::fabs(acc) > std::numeric_limits<float>::max())
        result = (acc < 0) ? -std::numeric_limits<float>::infinity()
                           :  std::numeric_limits<float>::infinity();
    return result;
}
