#include "pch.h"
#include "EffectiveBmpReaderClass.h"
#include <fstream>
#include <stdexcept>

namespace EffectiveMobileBmpReader
{
    namespace BmpUtils
    {
        struct BMPColorHeader
        {
            uint32_t red_mask{ 0x00ff0000u };
            uint32_t green_mask{ 0x0000ff00u };
            uint32_t blue_mask{ 0x000000ffu };
            uint32_t alpha_mask{ 0xff000000u };
            uint32_t color_space_type{ 0x73524742u };
            uint32_t unused[16]{ 0 };
        };

        static const WORD BmpHeaderFormatSignature = 0x4D42u;
        static const WCHAR BlackCharacter = 'B';
        static const WCHAR WhiteCharacter = '1';
        static const WCHAR GreyCharacter = '?';
        static const WCHAR TransparentCharacter = '-';
        static BMPColorHeader mExpectedColorHeader;


        static constexpr auto slide(const BMPRowSpan& s, std::size_t offset, std::size_t width)
        {
            return s.subspan(offset, offset + width <= s.size() ? width : 0U);
        }

        static void CheckColorHeader(BMPColorHeader &aBmpColorHeader)
        {
            if (mExpectedColorHeader.red_mask != aBmpColorHeader.red_mask ||
                mExpectedColorHeader.blue_mask != aBmpColorHeader.blue_mask ||
                mExpectedColorHeader.green_mask != aBmpColorHeader.green_mask ||
                mExpectedColorHeader.alpha_mask != aBmpColorHeader.alpha_mask)
            {
                throw std::runtime_error("Unexpected color mask format: The program expects the pixel data to be in the BGRA format.");
            }
            if ((aBmpColorHeader.color_space_type != mExpectedColorHeader.color_space_type) && (aBmpColorHeader.color_space_type != 0x57696E20u))
            {
                throw std::runtime_error("Unexpected color space type: The program expects sRGB values.");
            }
        }

        EffectiveBmpReader::EffectiveBmpReader()
            : OutputStream(&std::wcout)
        {
        }

        uint32_t EffectiveBmpReader::MakeStrideAligned(uint32_t aAlignStride) const
        {
            uint32_t aNewStride = mRowStride;
            while (aNewStride % aAlignStride != 0)
                aNewStride++;

            return aNewStride;
        }

        void EffectiveBmpReader::PrintRow(const BMPRowSpan& aRow) const
        {
            const WORD aStep = mBmpInfoHeader.biBitCount / 8;
            std::vector<WCHAR> aRowString;
            const int aCharStep = (UseSpaceBetweenCharacters) ? 2 : 1;
            aRowString.resize(mBmpInfoHeader.biWidth * aCharStep + 1);

            for (int i = 0; i < mBmpInfoHeader.biWidth; ++i)
            {
                uint32_t aValue = 0;
                memcpy(&aValue, aRow.data() + i * aStep, aStep);
                if ((aStep == 4) && (aValue & 0xff000000u) == 0)
                    aRowString[i * aCharStep] = TransparentCharacter;
                else
                {
                    aValue &= 0x00ffffffu;
                    if (aValue == 0)
                        aRowString[i * aCharStep] = BlackCharacter;
                    else if (aValue == 0x00ffffffu)
                        aRowString[i * aCharStep] = WhiteCharacter;
                    else
                        aRowString[i * aCharStep] = GreyCharacter;
                    if (UseSpaceBetweenCharacters)
                        aRowString[i * aCharStep + 1] = ' ';
                }
            }

            *OutputStream << aRowString.data() << std::endl;
        }
        
        void EffectiveBmpReader::AppendNewLine() const
        {
            *OutputStream << std::endl;
        }

        void EffectiveBmpReader::Open(LPCWSTR aFileName)
        {
            Close();

            std::ifstream inp{ aFileName, std::ios_base::binary };
            if (inp)
            {
                inp.read((char*)&mFileHeader, sizeof(mFileHeader));
                if (mFileHeader.bfType != BmpHeaderFormatSignature)
                {
                    throw std::runtime_error("Error: Unrecognized file format.");
                }
                inp.read((char*)&mBmpInfoHeader, sizeof(mBmpInfoHeader));

                if (mBmpInfoHeader.biBitCount == 32)
                {
                    if (mBmpInfoHeader.biSize >= (sizeof(BITMAPINFOHEADER) + sizeof(BMPColorHeader)))
                    {
                        BMPColorHeader aBmpColorHeader;
                        inp.read((char*)&aBmpColorHeader, sizeof(aBmpColorHeader));
                        CheckColorHeader(aBmpColorHeader);
                    }
                    else
                    {
                        throw std::runtime_error("Error: The BMP file does not seem to contain bit mask information.");
                    }
                }

                inp.seekg(mFileHeader.bfOffBits, inp.beg);

                if (mBmpInfoHeader.biBitCount == 32)
                {
                    mBmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER) + sizeof(BMPColorHeader);
                    mFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(BMPColorHeader);
                }
                else
                {
                    mBmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
                    mFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
                }
                mFileHeader.bfSize = mFileHeader.bfOffBits;

                if ((mBmpInfoHeader.biCompression != BI_RGB) && (mBmpInfoHeader.biCompression != BI_BITFIELDS))
                {
                    throw std::runtime_error("Error: Compressed images are not supported.");
                }

                LONG aHeight = mBmpInfoHeader.biHeight;
                const bool aHeightIsNegative = aHeight < 0;
                if (aHeightIsNegative)
                    aHeight = -aHeight;

                mRowStride = mBmpInfoHeader.biWidth * mBmpInfoHeader.biBitCount / 8;
                mData.resize(mRowStride * aHeight);

                if (mBmpInfoHeader.biWidth % 4 == 0)
                {
                    if (aHeightIsNegative)
                        inp.read((char*)mData.data(), mData.size());
                    else
                    {
                        for (int y = 0; y < aHeight; ++y)
                        {
                            LONG aYOffset = (aHeightIsNegative) ? y : aHeight - y - 1;
                            inp.read((char*)(mData.data() + mRowStride * aYOffset), mRowStride);
                        }
                    }
                    mFileHeader.bfSize += static_cast<uint32_t>(mData.size());
                }
                else
                {
                    const uint32_t aNewStride = MakeStrideAligned(4);
                    BMPRawData aPaddingRow(aNewStride - mRowStride);

                    for (int y = 0; y < aHeight; ++y)
                    {
                        LONG aYOffset = (aHeightIsNegative) ? y : aHeight - y - 1;
                        inp.read((char*)(mData.data() + mRowStride * aYOffset), mRowStride);
                        inp.read((char*)aPaddingRow.data(), aPaddingRow.size());
                    }
                    mFileHeader.bfSize += static_cast<uint32_t>(mData.size()) + aHeight * static_cast<uint32_t>(aPaddingRow.size());
                }
            }
            else
            {
                throw std::runtime_error("Error: Unable to open the input image file.");
            }
        }

        void EffectiveBmpReader::Display()
        {
            if (mRowStride && mData.size())
            {
                LONG aHeight = mBmpInfoHeader.biHeight;
                if (aHeight < 0)
                    aHeight = -aHeight;

                for (int y = 0; y < aHeight; ++y)
                {
                    auto aRow = slide(std::span{ mData }, mRowStride * y, mRowStride);
                    if (aRow.empty())
                        break;

                    PrintRow(aRow);
                }
            }
        }

        void EffectiveBmpReader::Close()
        {
            mRowStride = 0;
            mData.clear();
        }

        void EffectiveBmpReader::SetOutputStream(std::wostream& aOutputStream)
        {
            OutputStream = &aOutputStream;
        }
    }
}