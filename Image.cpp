#include "Image.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace
{
    constexpr int kFileHeaderSize = 14;   // BITMAPFILEHEADER
    constexpr int kInfoHeaderSize = 40;   // BITMAPINFOHEADER
    constexpr std::uint32_t kBiRgb = 0;
    constexpr std::uint32_t kBiBitfields = 3;

    // Защита от переполнения и от огромных выделений на испорченных файлах.
    constexpr std::int64_t kMaxPixels = std::int64_t(1) << 28;

    // Длина строки в байтах, выровненная до 4 байт, как требует формат BMP.
    std::int64_t RowStride(std::int64_t width, int bitsPerPixel)
    {
        return ((width * bitsPerPixel + 31) / 32) * 4;
    }

    // BMP хранит числа в little-endian независимо от платформы.
    std::uint16_t ReadU16(const std::uint8_t* p) { return std::uint16_t(p[0] | (p[1] << 8)); }
    std::uint32_t ReadU32(const std::uint8_t* p)
    {
        return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
               (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
    }
    std::int32_t ReadI32(const std::uint8_t* p) { return std::int32_t(ReadU32(p)); }

    void WriteU16(std::uint8_t* p, std::uint16_t v)
    {
        p[0] = std::uint8_t(v);
        p[1] = std::uint8_t(v >> 8);
    }
    void WriteU32(std::uint8_t* p, std::uint32_t v)
    {
        p[0] = std::uint8_t(v);
        p[1] = std::uint8_t(v >> 8);
        p[2] = std::uint8_t(v >> 16);
        p[3] = std::uint8_t(v >> 24);
    }

    bool Fail(std::string* error, const std::string& message)
    {
        if (error) *error = message;
        return false;
    }
}

std::uint8_t ToByte(float v)
{
    const float clamped = std::min(1.0f, std::max(0.0f, v));
    return std::uint8_t(std::lround(clamped * 255.0f));
}

float FromByte(std::uint8_t v)
{
    return float(v) / 255.0f;
}

Image::Image(int width, int height)
{
    if (width < 0 || height < 0)
        throw std::invalid_argument("Image: negative size");
    m_width = width;
    m_height = height;
    m_colours.resize(std::size_t(width) * std::size_t(height));
}

std::size_t Image::Index(int x, int y) const
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height)
        throw std::out_of_range("Image: pixel coordinates out of range");
    return std::size_t(y) * std::size_t(m_width) + std::size_t(x);
}

Colour Image::GetColour(int x, int y) const
{
    return m_colours[Index(x, y)];
}

void Image::SetColour(const Colour& colour, int x, int y)
{
    m_colours[Index(x, y)] = colour;
}

bool Image::Export(const std::string& path) const
{
    const std::int64_t stride = RowStride(m_width, 24);
    const std::int64_t imageSize = stride * m_height;
    const std::int64_t fileSize = kFileHeaderSize + kInfoHeaderSize + imageSize;
    if (fileSize > 0xFFFFFFFFll)
        return false;

    std::uint8_t header[kFileHeaderSize + kInfoHeaderSize] = {};

    // BITMAPFILEHEADER
    header[0] = 'B';
    header[1] = 'M';
    WriteU32(header + 2, std::uint32_t(fileSize));                       // bfSize
    WriteU32(header + 10, kFileHeaderSize + kInfoHeaderSize);            // bfOffBits: пиксели сразу после заголовков

    // BITMAPINFOHEADER
    std::uint8_t* info = header + kFileHeaderSize;
    WriteU32(info + 0, kInfoHeaderSize);                                 // biSize
    WriteU32(info + 4, std::uint32_t(m_width));                          // biWidth
    WriteU32(info + 8, std::uint32_t(m_height));                         // biHeight > 0: строки снизу вверх
    WriteU16(info + 12, 1);                                              // biPlanes
    WriteU16(info + 14, 24);                                             // biBitCount
    WriteU32(info + 16, kBiRgb);                                         // biCompression
    WriteU32(info + 20, std::uint32_t(imageSize));                       // biSizeImage
    WriteU32(info + 24, 2835);                                           // 72 DPI
    WriteU32(info + 28, 2835);

    std::ofstream f(path, std::ios::binary);
    if (!f)
        return false;
    f.write(reinterpret_cast<const char*>(header), sizeof(header));

    std::vector<std::uint8_t> row(std::size_t(stride), 0);              // хвост строки — нули выравнивания
    for (int y = m_height - 1; y >= 0; --y)                              // первой в файле идёт нижняя строка
    {
        for (int x = 0; x < m_width; ++x)
        {
            const Colour& c = m_colours[std::size_t(y) * m_width + x];
            row[3 * x + 0] = ToByte(c.b);                                // в файле порядок BGR
            row[3 * x + 1] = ToByte(c.g);
            row[3 * x + 2] = ToByte(c.r);
        }
        f.write(reinterpret_cast<const char*>(row.data()), std::streamsize(row.size()));
    }
    return bool(f);
}

bool Image::ReadImage(const std::string& path, std::string* error)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return Fail(error, "cannot open file: " + path);
    const std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(f)),
                                         std::istreambuf_iterator<char>());

    if (data.size() < std::size_t(kFileHeaderSize + kInfoHeaderSize))
        return Fail(error, "file is too small to be a BMP");
    if (data[0] != 'B' || data[1] != 'M')
        return Fail(error, "not a BMP file (no 'BM' signature)");

    const std::uint32_t pixelOffset = ReadU32(&data[10]);
    const std::uint8_t* info = &data[kFileHeaderSize];
    const std::uint32_t infoSize = ReadU32(info + 0);
    if (infoSize < std::uint32_t(kInfoHeaderSize))
        return Fail(error, "unsupported BMP header (older than BITMAPINFOHEADER)");

    const std::int32_t width = ReadI32(info + 4);
    const std::int32_t rawHeight = ReadI32(info + 8);
    const std::uint16_t planes = ReadU16(info + 12);
    const std::uint16_t bits = ReadU16(info + 14);
    const std::uint32_t compression = ReadU32(info + 16);

    if (planes != 1)
        return Fail(error, "invalid BMP: planes != 1");
    if (bits != 24 && bits != 32)
        return Fail(error, "unsupported bit depth " + std::to_string(bits) + " (only 24 and 32 are supported)");
    if (compression != kBiRgb && !(bits == 32 && compression == kBiBitfields))
        return Fail(error, "compressed BMP is not supported");
    if (width <= 0 || rawHeight == 0 || rawHeight == INT32_MIN)
        return Fail(error, "invalid BMP size");

    const bool topDown = rawHeight < 0;                                  // отрицательная высота: строки сверху вниз
    const std::int64_t height = topDown ? -std::int64_t(rawHeight) : rawHeight;
    if (std::int64_t(width) * height > kMaxPixels)
        return Fail(error, "BMP is too large");

    // 32 бита с BI_BITFIELDS поддерживаем только в стандартной раскладке BGRA.
    if (compression == kBiBitfields)
    {
        if (data.size() < std::size_t(kFileHeaderSize + kInfoHeaderSize + 12))
            return Fail(error, "truncated BMP bit masks");
        const std::uint8_t* masks = info + kInfoHeaderSize;              // для V4/V5 маски лежат там же
        if (ReadU32(masks) != 0x00FF0000u || ReadU32(masks + 4) != 0x0000FF00u ||
            ReadU32(masks + 8) != 0x000000FFu)
            return Fail(error, "unsupported 32-bit channel masks");
    }

    const std::int64_t stride = RowStride(width, bits);
    if (std::int64_t(pixelOffset) + stride * height > std::int64_t(data.size()))
        return Fail(error, "truncated BMP pixel data");

    const int bytesPerPixel = bits / 8;
    std::vector<Colour> colours(std::size_t(width) * std::size_t(height));
    for (std::int64_t fileRow = 0; fileRow < height; ++fileRow)
    {
        const std::uint8_t* src = &data[std::size_t(pixelOffset + fileRow * stride)];
        const std::int64_t y = topDown ? fileRow : height - 1 - fileRow;
        Colour* dst = &colours[std::size_t(y * width)];
        for (std::int32_t x = 0; x < width; ++x, src += bytesPerPixel)
            dst[x] = Colour(FromByte(src[2]), FromByte(src[1]), FromByte(src[0]));   // BGR(A) -> RGB
    }

    m_width = width;
    m_height = int(height);
    m_colours = std::move(colours);
    return true;
}
