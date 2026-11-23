// Для каждого пикселя исходного изображения находит ближайший цвет
// (евклидово расстояние в RGB) среди цветов изображения-палитры
// и записывает результат в новый BMP.
//
//   BitMap_STL <source.bmp> <palette.bmp> <output.bmp>
//
// Без аргументов — демонстрация: строит градиент, сохраняет его,
// читает обратно и перекрашивает пример фотографии цветами градиента.

#include "Image.h"
#include "NearestColour.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <string>

namespace
{
    // Градиент: красный растёт слева направо, зелёный убывает, синий растёт сверху вниз.
    Image MakeGradient(int width, int height)
    {
        Image image(width, height);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                const float u = float(x) / float(width - 1);
                const float v = float(y) / float(height - 1);
                image.SetColour(Colour(u, 1.0f - u, v), x, y);
            }
        return image;
    }

    // Сравнение после перевода в байты: именно с такой точностью хранит BMP.
    bool SameBytes(const Image& a, const Image& b)
    {
        if (a.Width() != b.Width() || a.Height() != b.Height())
            return false;
        for (std::size_t i = 0; i < a.Pixels().size(); ++i)
            if (ToRgb8(a.Pixels()[i]) != ToRgb8(b.Pixels()[i]))
                return false;
        return true;
    }

    bool Load(Image& image, const std::string& path)
    {
        std::string error;
        if (!image.ReadImage(path, &error))
        {
            std::cerr << "Cannot read " << path << ": " << error << "\n";
            return false;
        }
        std::cout << "Read " << path << ": " << image.Width() << " x " << image.Height() << "\n";
        return true;
    }

    int Run(const std::string& sourcePath, const std::string& palettePath, const std::string& outputPath)
    {
        Image source, palette;
        if (!Load(source, sourcePath) || !Load(palette, palettePath))
            return 1;

        const auto t0 = std::chrono::steady_clock::now();
        const Image result = FindNearestColours(source, palette);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "Nearest colours found in " << ms << " ms\n";

        if (!result.Export(outputPath))
        {
            std::cerr << "Cannot write " << outputPath << "\n";
            return 1;
        }
        std::cout << "Written " << outputPath << "\n";
        return 0;
    }

    int Demo()
    {
        const Image gradient = MakeGradient(640, 480);
        if (!gradient.Export("gradient.bmp"))
        {
            std::cerr << "Cannot write gradient.bmp\n";
            return 1;
        }

        Image check;
        if (!Load(check, "gradient.bmp"))
            return 1;
        std::cout << "Round trip gradient.bmp: "
                  << (SameBytes(check, gradient) ? "identical" : "DIFFERENT") << "\n";

        // Фотография из репозитория, перекрашенная цветами градиента.
        return Run("sample_1280x853.bmp", "gradient.bmp", "sample_in_gradient_colours.bmp");
    }
}

int main(int argc, char** argv)
{
    try
    {
        if (argc == 1)
            return Demo();
        if (argc == 4)
            return Run(argv[1], argv[2], argv[3]);

        std::cerr << "Usage: " << argv[0] << " <source.bmp> <palette.bmp> <output.bmp>\n"
                  << "       " << argv[0] << "            (demo)\n";
        return 2;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
