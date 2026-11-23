#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Цвет в линейной шкале [0, 1] по каждому каналу.
struct Colour
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    Colour() = default;
    Colour(float r, float g, float b) : r(r), g(g), b(b) {}

    friend bool operator==(const Colour& a, const Colour& b)
    {
        return a.r == b.r && a.g == b.g && a.b == b.b;
    }
    friend bool operator!=(const Colour& a, const Colour& b) { return !(a == b); }
};

// Перевод канала [0, 1] <-> байт [0, 255] с округлением и ограничением.
// Обратный перевод точный: ToByte(FromByte(v)) == v для любого байта.
std::uint8_t ToByte(float v);
float FromByte(std::uint8_t v);

// Изображение RGB. Пиксели хранятся построчно, y = 0 — ВЕРХНЯЯ строка
// (в файле BMP строки идут снизу вверх; переворот делают Export/ReadImage).
class Image
{
public:
    Image() = default;
    Image(int width, int height);

    int Width() const { return m_width; }
    int Height() const { return m_height; }
    bool Empty() const { return m_colours.empty(); }

    Colour GetColour(int x, int y) const;
    void SetColour(const Colour& colour, int x, int y);

    const std::vector<Colour>& Pixels() const { return m_colours; }

    // Запись в 24-битный BMP (BITMAPINFOHEADER, без сжатия).
    // Возвращает false, если файл не удалось открыть или записать.
    bool Export(const std::string& path) const;

    // Чтение несжатого BMP: 24 или 32 бита на пиксель, любой заголовок
    // (BITMAPINFOHEADER, V4, V5), строки снизу вверх или сверху вниз.
    // При ошибке возвращает false, текст ошибки — в error (если передан),
    // само изображение не меняется.
    bool ReadImage(const std::string& path, std::string* error = nullptr);

private:
    std::size_t Index(int x, int y) const;

    int m_width = 0;
    int m_height = 0;
    std::vector<Colour> m_colours;
};
