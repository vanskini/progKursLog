#pragma once
#include <vector>

class Calc {
public:
    // Сумма квадратов значений вектора.
    // Переполнение вверх -> +INF, вниз -> -INF.
    float sumSquares(const std::vector<float>& v) const;
};
