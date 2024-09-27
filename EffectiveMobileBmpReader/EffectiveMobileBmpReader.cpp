// EffectiveMobileBmpReader.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "EffectiveBmpReaderClass.h"

#include <fstream>
#include <iostream>

using namespace EffectiveMobileBmpReader::BmpUtils;

static LPCWSTR mUsageString = L"Usage: EffectiveMobileBmpReader BmpFile1 [BmpFile2] ...";

int _tmain(int argc, _TCHAR* argv[])
{
    (void)_setmode(_fileno(stdin), _O_U16TEXT);
    (void)_setmode(_fileno(stdout), _O_U16TEXT);
    (void)_setmode(_fileno(stderr), _O_U16TEXT);

    if (argc > 1)
    {
        const bool aUseSpace = _tcscmp(argv[1], __TEXT("1")) == 0;
        const int aParamStart = (aUseSpace) ? 2 : 1;

        if (argc > aParamStart)
        {
            EffectiveBmpReader aBmpReader;
            aBmpReader.UseSpaceBetweenCharacters = aUseSpace;

            for (int i = aParamStart; i < argc; i++)
            {
                const LPCTSTR aFileName = argv[i];
                
                aBmpReader.Open(aFileName);
                aBmpReader.Display();
                aBmpReader.Close();
                aBmpReader.AppendNewLine();
                aBmpReader.AppendNewLine();
            }

            return 0;
        }
    }

    std::wcout << mUsageString << std::endl;

    return 1;
}
