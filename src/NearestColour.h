#pragma once

#include "Image.h"

#include <array>
#include <cstdint>
#include <vector>

// Цвет в байтах RGB: на этом уровне сравнение точное, без ошибок float.
using Rgb8 = std::array<std::uint8_t, 3>;

Rgb8 ToRgb8(const Colour& c);
Colour FromRgb8(const Rgb8& c);

// Квадрат евклидова расстояния в пространстве RGB (целочисленно, без sqrt).
inline int DistanceSq(const Rgb8& a, const Rgb8& b)
{
    const int dr = int(a[0]) - int(b[0]);
    const int dg = int(a[1]) - int(b[1]);
    const int db = int(a[2]) - int(b[2]);
    return dr * dr + dg * dg + db * db;
}

// k-d дерево над набором цветов палитры: поиск ближайшего цвета
// за O(log n) в среднем вместо полного перебора палитры.
class ColourKdTree
{
public:
    explicit ColourKdTree(std::vector<Rgb8> colours);

    bool Empty() const { return m_points.empty(); }
    std::size_t Size() const { return m_points.size(); }

    // Ближайший цвет палитры. Палитра не должна быть пустой.
    Rgb8 Nearest(const Rgb8& query) const;

private:
    void Build(std::size_t lo, std::size_t hi, int axis);
    void Search(std::size_t lo, std::size_t hi, int axis, const Rgb8& q,
                std::size_t& best, int& bestDist) const;

    // Дерево неявное: узел — отрезок [lo, hi), его корень — медиана mid.
    // Для каждого узла храним ограничивающий параллелепипед его точек
    // (по индексу mid): поиск отсекает поддерево, если весь параллелепипед
    // дальше текущего лучшего кандидата. Одной плоскости разбиения для этого
    // мало, когда запрос лежит далеко от палитры.
    struct Box { Rgb8 lo, hi; };

    std::vector<Rgb8> m_points;
    std::vector<Box> m_boxes;
};

// Для каждого пикселя source — ближайший по RGB цвет, который встречается в palette.
// Результат имеет размер source. Если palette пуста, бросает std::invalid_argument.
Image FindNearestColours(const Image& source, const Image& palette);

// Эталон полным перебором, для тестов: O(пикселей source × пикселей palette).
Rgb8 NearestColourBruteForce(const Rgb8& query, const std::vector<Rgb8>& palette);
