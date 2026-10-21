// Тесты чтения/записи BMP и поиска ближайшего цвета.
// Запуск из корня репозитория (нужны sample_1280x853.bmp и obraz-A.jpg).

#include "Image.h"
#include "NearestColour.h"

#include <cstdio>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace
{
    int g_checks = 0;
    int g_failed = 0;

#define CHECK(cond)                                                                  \
    do {                                                                             \
        ++g_checks;                                                                  \
        if (!(cond)) { ++g_failed; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

    std::vector<std::uint8_t> ReadFile(const std::string& path)
    {
        std::ifstream f(path, std::ios::binary);
        return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }

    void WriteFile(const std::string& path, const std::vector<std::uint8_t>& data)
    {
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
    }

    std::uint32_t U32(const std::vector<std::uint8_t>& d, std::size_t at)
    {
        return d[at] | (d[at + 1] << 8) | (d[at + 2] << 16) | (std::uint32_t(d[at + 3]) << 24);
    }

    Image RandomImage(int w, int h, std::mt19937& rng)
    {
        std::uniform_int_distribution<int> byte(0, 255);
        Image img(w, h);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                img.SetColour(FromRgb8({ std::uint8_t(byte(rng)), std::uint8_t(byte(rng)), std::uint8_t(byte(rng)) }), x, y);
        return img;
    }

    bool SameBytes(const Image& a, const Image& b)
    {
        if (a.Width() != b.Width() || a.Height() != b.Height())
            return false;
        for (std::size_t i = 0; i < a.Pixels().size(); ++i)
            if (ToRgb8(a.Pixels()[i]) != ToRgb8(b.Pixels()[i]))
                return false;
        return true;
    }

    // 1. Запись и чтение дают то же изображение при любой ширине (разное выравнивание строк).
    void TestRoundTrip()
    {
        std::printf("[1] round trip, widths 1..9 and 641\n");
        std::mt19937 rng(1);
        for (int w : { 1, 2, 3, 4, 5, 6, 7, 8, 9, 641 })
        {
            const Image img = RandomImage(w, 7, rng);
            CHECK(img.Export("test_roundtrip.bmp"));
            Image back;
            std::string error;
            CHECK(back.ReadImage("test_roundtrip.bmp", &error));
            CHECK(SameBytes(img, back));
        }
        std::remove("test_roundtrip.bmp");
    }

    // 2. Заголовок записанного файла соответствует формату.
    void TestHeader()
    {
        std::printf("[2] header of an exported 5x3 image\n");
        Image img(5, 3);
        CHECK(img.Export("test_header.bmp"));
        const auto d = ReadFile("test_header.bmp");
        const std::uint32_t stride = 16;                        // 5*3 = 15 байт, выровнено до 16
        CHECK(d.size() == 54 + stride * 3);
        CHECK(d[0] == 'B' && d[1] == 'M');
        CHECK(U32(d, 2) == d.size());                           // bfSize
        CHECK(U32(d, 10) == 54);                                // bfOffBits (в старой версии был 0)
        CHECK(U32(d, 14) == 40);                                // biSize
        CHECK(U32(d, 18) == 5 && U32(d, 22) == 3);              // ширина и высота
        CHECK((d[28] | (d[29] << 8)) == 24);                    // бит на пиксель
        std::remove("test_header.bmp");
    }

    // 3. Порядок каналов и строк: красный пиксель в левом верхнем углу.
    void TestOrientationAndChannels()
    {
        std::printf("[3] channel order (BGR in file) and row order (bottom-up)\n");
        Image img(2, 2);
        img.SetColour(Colour(1, 0, 0), 0, 0);                   // красный, верх-лево
        img.SetColour(Colour(0, 0, 1), 1, 1);                   // синий, низ-право
        CHECK(img.Export("test_orient.bmp"));
        const auto d = ReadFile("test_orient.bmp");
        // Первая строка файла — нижняя строка изображения: (0,1) чёрный, (1,1) синий.
        CHECK(d[54 + 3] == 255 && d[54 + 4] == 0 && d[54 + 5] == 0);   // B G R синего
        // Вторая строка файла (смещение 54 + 8) — верхняя: (0,0) красный.
        CHECK(d[62 + 0] == 0 && d[62 + 1] == 0 && d[62 + 2] == 255);   // B G R красного
        std::remove("test_orient.bmp");
    }

    // 4. Чтение 32-битного BMP, записанного сверху вниз (отрицательная высота).
    void TestTopDown32()
    {
        std::printf("[4] 32-bit top-down BMP\n");
        const int w = 3, h = 2;
        std::vector<std::uint8_t> d(54 + w * h * 4, 0);
        d[0] = 'B'; d[1] = 'M';
        auto put32 = [&](std::size_t at, std::uint32_t v) { for (int i = 0; i < 4; ++i) d[at + i] = std::uint8_t(v >> (8 * i)); };
        put32(2, std::uint32_t(d.size()));
        put32(10, 54);
        put32(14, 40);
        put32(18, w);
        put32(22, std::uint32_t(-h));                           // сверху вниз
        d[26] = 1; d[28] = 32;
        // Первая строка файла = верхняя строка: пиксель (0,0) = (10, 20, 30)
        d[54 + 0] = 30; d[54 + 1] = 20; d[54 + 2] = 10;
        // Вторая строка, пиксель (2,1) = (200, 100, 50)
        const std::size_t p = 54 + (w * 4) + 2 * 4;
        d[p + 0] = 50; d[p + 1] = 100; d[p + 2] = 200;
        WriteFile("test_topdown.bmp", d);

        Image img;
        std::string error;
        CHECK(img.ReadImage("test_topdown.bmp", &error));
        CHECK(img.Width() == 3 && img.Height() == 2);
        CHECK((ToRgb8(img.GetColour(0, 0)) == Rgb8{ 10, 20, 30 }));
        CHECK((ToRgb8(img.GetColour(2, 1)) == Rgb8{ 200, 100, 50 }));
        std::remove("test_topdown.bmp");
    }

    // 5. Пример из репозитория: заголовок V5 (124 байта), пиксели со смещения 138.
    void TestSampleV5()
    {
        std::printf("[5] sample_1280x853.bmp (BITMAPV5HEADER, pixel offset 138)\n");
        Image img;
        std::string error;
        CHECK(img.ReadImage("sample_1280x853.bmp", &error));
        if (img.Empty()) { std::printf("  (%s)\n", error.c_str()); return; }
        CHECK(img.Width() == 1280 && img.Height() == 853);
        // Эталонные значения получены независимо (Pillow).
        CHECK((ToRgb8(img.GetColour(0, 0)) == Rgb8{ 178, 132, 145 }));
        CHECK((ToRgb8(img.GetColour(1279, 0)) == Rgb8{ 125, 90, 114 }));
        CHECK((ToRgb8(img.GetColour(0, 852)) == Rgb8{ 7, 6, 12 }));
        CHECK((ToRgb8(img.GetColour(1279, 852)) == Rgb8{ 104, 108, 13 }));
        CHECK((ToRgb8(img.GetColour(640, 426)) == Rgb8{ 233, 196, 187 }));
        CHECK((ToRgb8(img.GetColour(123, 456)) == Rgb8{ 59, 21, 0 }));
    }

    // 6. Некорректные файлы отклоняются, изображение не портится.
    void TestInvalidInput()
    {
        std::printf("[6] invalid input is rejected\n");
        Image img(2, 2);
        std::string error;
        CHECK(!img.ReadImage("no_such_file.bmp", &error));
        CHECK(!img.ReadImage("obraz-A.jpg", &error));           // JPEG, не BMP
        CHECK(img.Width() == 2 && img.Height() == 2);           // прежнее содержимое на месте

        auto d = ReadFile("sample_1280x853.bmp");
        d.resize(d.size() / 2);                                 // обрезанный файл
        WriteFile("test_truncated.bmp", d);
        CHECK(!img.ReadImage("test_truncated.bmp", &error));
        std::remove("test_truncated.bmp");
    }

    // 7. k-d дерево находит ближайший цвет так же, как полный перебор.
    void TestKdTreeAgainstBruteForce()
    {
        std::printf("[7] k-d tree vs brute force: random palettes and a surface palette\n");
        std::mt19937 rng(7);
        std::uniform_int_distribution<int> byte(0, 255);
        auto random = [&] { return Rgb8{ std::uint8_t(byte(rng)), std::uint8_t(byte(rng)), std::uint8_t(byte(rng)) }; };

        for (int size : { 1, 2, 17, 500, 5000 })
        {
            std::vector<Rgb8> palette(size);
            for (auto& c : palette) c = random();
            const ColourKdTree tree(palette);
            int mismatches = 0;
            for (int i = 0; i < 4000; ++i)
            {
                const Rgb8 q = random();
                // Сравниваем расстояния: при равноудалённых цветах допустим любой из них.
                if (DistanceSq(q, tree.Nearest(q)) != DistanceSq(q, NearestColourBruteForce(q, palette)))
                    ++mismatches;
            }
            CHECK(mismatches == 0);
        }

        // Палитра на поверхности r + g = 255 (как у градиента в демо), запросы
        // далеко от неё — случай, где отсечение по параллелепипедам работает больше всего.
        std::vector<Rgb8> surface;
        for (int r = 0; r < 256; r += 3)
            for (int b = 0; b < 256; b += 5)
                surface.push_back({ std::uint8_t(r), std::uint8_t(255 - r), std::uint8_t(b) });
        const ColourKdTree tree(surface);
        int mismatches = 0;
        for (int i = 0; i < 4000; ++i)
        {
            const Rgb8 q = random();
            if (DistanceSq(q, tree.Nearest(q)) != DistanceSq(q, NearestColourBruteForce(q, surface)))
                ++mismatches;
        }
        CHECK(mismatches == 0);
    }

    // 8. Полный конвейер: цвет из палитры отображается сам в себя,
    //    каждый пиксель результата — цвет палитры.
    void TestFindNearestColours()
    {
        std::printf("[8] FindNearestColours on images\n");
        std::mt19937 rng(8);
        const Image palette = RandomImage(16, 16, rng);
        const Image same = FindNearestColours(palette, palette);
        CHECK(SameBytes(same, palette));

        const Image source = RandomImage(40, 30, rng);
        const Image result = FindNearestColours(source, palette);
        CHECK(result.Width() == 40 && result.Height() == 30);

        std::vector<Rgb8> paletteColours;
        for (const Colour& c : palette.Pixels()) paletteColours.push_back(ToRgb8(c));
        int bad = 0;
        for (std::size_t i = 0; i < source.Pixels().size(); ++i)
        {
            const Rgb8 q = ToRgb8(source.Pixels()[i]);
            const Rgb8 got = ToRgb8(result.Pixels()[i]);
            if (DistanceSq(q, got) != DistanceSq(q, NearestColourBruteForce(q, paletteColours)))
                ++bad;
        }
        CHECK(bad == 0);
    }
}

int main()
{
    TestRoundTrip();
    TestHeader();
    TestOrientationAndChannels();
    TestTopDown32();
    TestSampleV5();
    TestInvalidInput();
    TestKdTreeAgainstBruteForce();
    TestFindNearestColours();
    std::printf("%s: checks %d, failed %d\n", g_failed ? "FAILED" : "OK", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
