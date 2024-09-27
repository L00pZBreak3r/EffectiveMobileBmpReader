#pragma once

#include <iostream>
#include <vector>
#include <span>

namespace EffectiveMobileBmpReader
{
namespace BmpUtils
{

using BMPRawData = std::vector<uint8_t>;
using BMPRowSpan = std::span<uint8_t>;

class EffectiveBmpReader
{
    BITMAPFILEHEADER mFileHeader;
    BITMAPINFOHEADER mBmpInfoHeader;
    BMPRawData mData;
    uint32_t mRowStride{ 0 };
    std::wostream* OutputStream;

    uint32_t MakeStrideAligned(uint32_t aAlignStride) const;
    void PrintRow(const BMPRowSpan& aRow) const;
public:
    bool UseSpaceBetweenCharacters{ false };

    EffectiveBmpReader();
    EffectiveBmpReader(const EffectiveBmpReader&) = delete;
    EffectiveBmpReader(EffectiveBmpReader&&) = delete;
    EffectiveBmpReader& operator=(const EffectiveBmpReader& other) = delete;

    void Open(LPCWSTR aFileName);
    void Display();
    void Close();
    void SetOutputStream(std::wostream& aOutputStream);
    void AppendNewLine() const;
};

}
}