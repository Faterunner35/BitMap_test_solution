#include "NearestColour.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace
{
    std::uint32_t Pack(const Rgb8& c)
    {
        return (std::uint32_t(c[0]) << 16) | (std::uint32_t(c[1]) << 8) | std::uint32_t(c[2]);
    }

    Rgb8 Unpack(std::uint32_t v)
    {
        return { std::uint8_t(v >> 16), std::uint8_t(v >> 8), std::uint8_t(v) };
    }

    // Уникальные цвета изображения: в фотографии их обычно в разы меньше, чем пикселей.
    std::vector<Rgb8> UniqueColours(const Image& image)
    {
        std::vector<std::uint32_t> packed;
        packed.reserve(image.Pixels().size());
        for (const Colour& c : image.Pixels())
            packed.push_back(Pack(ToRgb8(c)));
        std::sort(packed.begin(), packed.end());
        packed.erase(std::unique(packed.begin(), packed.end()), packed.end());

        std::vector<Rgb8> result;
        result.reserve(packed.size());
        for (std::uint32_t v : packed)
            result.push_back(Unpack(v));
        return result;
    }
}

Rgb8 ToRgb8(const Colour& c)
{
    return { ToByte(c.r), ToByte(c.g), ToByte(c.b) };
}

Colour FromRgb8(const Rgb8& c)
{
    return Colour(FromByte(c[0]), FromByte(c[1]), FromByte(c[2]));
}

ColourKdTree::ColourKdTree(std::vector<Rgb8> colours)
    : m_points(std::move(colours)), m_boxes(m_points.size())
{
    Build(0, m_points.size(), 0);
}

void ColourKdTree::Build(std::size_t lo, std::size_t hi, int axis)
{
    if (lo >= hi)
        return;
    const std::size_t mid = lo + (hi - lo) / 2;

    Box box{ { 255, 255, 255 }, { 0, 0, 0 } };
    for (std::size_t i = lo; i < hi; ++i)
        for (int k = 0; k < 3; ++k)
        {
            box.lo[k] = std::min(box.lo[k], m_points[i][k]);
            box.hi[k] = std::max(box.hi[k], m_points[i][k]);
        }
    m_boxes[mid] = box;

    if (hi - lo == 1)
        return;
    std::nth_element(m_points.begin() + lo, m_points.begin() + mid, m_points.begin() + hi,
                     [axis](const Rgb8& a, const Rgb8& b) { return a[axis] < b[axis]; });
    const int next = (axis + 1) % 3;
    Build(lo, mid, next);
    Build(mid + 1, hi, next);
}

void ColourKdTree::Search(std::size_t lo, std::size_t hi, int axis, const Rgb8& q,
                          std::size_t& best, int& bestDist) const
{
    if (lo >= hi)
        return;
    const std::size_t mid = lo + (hi - lo) / 2;

    // Квадрат расстояния от запроса до параллелепипеда узла.
    const Box& box = m_boxes[mid];
    int boxDist = 0;
    for (int k = 0; k < 3; ++k)
    {
        const int below = int(box.lo[k]) - int(q[k]);
        const int above = int(q[k]) - int(box.hi[k]);
        const int e = below > 0 ? below : (above > 0 ? above : 0);
        boxDist += e * e;
    }
    if (boxDist >= bestDist)
        return;

    const Rgb8& p = m_points[mid];
    const int d = DistanceSq(q, p);
    if (d < bestDist)
    {
        bestDist = d;
        best = mid;
    }

    const int diff = int(q[axis]) - int(p[axis]);
    const int next = (axis + 1) % 3;
    // Сначала поддерево со стороны запроса, затем другое — только если
    // плоскость разбиения ближе текущего лучшего расстояния.
    if (diff < 0)
    {
        Search(lo, mid, next, q, best, bestDist);
        if (diff * diff < bestDist)
            Search(mid + 1, hi, next, q, best, bestDist);
    }
    else
    {
        Search(mid + 1, hi, next, q, best, bestDist);
        if (diff * diff < bestDist)
            Search(lo, mid, next, q, best, bestDist);
    }
}

Rgb8 ColourKdTree::Nearest(const Rgb8& query) const
{
    if (m_points.empty())
        throw std::logic_error("ColourKdTree::Nearest on empty palette");
    std::size_t best = 0;
    int bestDist = std::numeric_limits<int>::max();
    Search(0, m_points.size(), 0, query, best, bestDist);
    return m_points[best];
}

Rgb8 NearestColourBruteForce(const Rgb8& query, const std::vector<Rgb8>& palette)
{
    if (palette.empty())
        throw std::invalid_argument("NearestColourBruteForce: empty palette");
    Rgb8 best = palette.front();
    int bestDist = std::numeric_limits<int>::max();
    for (const Rgb8& p : palette)
    {
        const int d = DistanceSq(query, p);
        if (d < bestDist)
        {
            bestDist = d;
            best = p;
        }
    }
    return best;
}

Image FindNearestColours(const Image& source, const Image& palette)
{
    if (palette.Empty())
        throw std::invalid_argument("FindNearestColours: palette image is empty");

    const ColourKdTree tree(UniqueColours(palette));

    // Один и тот же цвет source ищем в дереве один раз.
    std::unordered_map<std::uint32_t, Rgb8> cache;
    cache.reserve(1 << 16);

    Image result(source.Width(), source.Height());
    for (int y = 0; y < source.Height(); ++y)
    {
        for (int x = 0; x < source.Width(); ++x)
        {
            const Rgb8 c = ToRgb8(source.GetColour(x, y));
            const std::uint32_t key = Pack(c);
            auto it = cache.find(key);
            if (it == cache.end())
                it = cache.emplace(key, tree.Nearest(c)).first;
            result.SetColour(FromRgb8(it->second), x, y);
        }
    }
    return result;
}
