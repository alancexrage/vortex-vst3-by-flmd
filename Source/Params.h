#pragma once
#include <JuceHeader.h>

namespace VortexParams
{
    inline constexpr const char* pRate    = "rate";
    inline constexpr const char* pSize    = "size";
    inline constexpr const char* pScan    = "scan";
    inline constexpr const char* pHarmony = "harmony";
    inline constexpr const char* pChaos   = "chaos";
    inline constexpr const char* pMix     = "mix";
    inline constexpr const char* pFreeze  = "freeze";
    inline constexpr const char* pKey     = "key";
    inline constexpr const char* pScale   = "scale";
    inline constexpr const char* pBuffer  = "buffer";

    // Шаг сетки в четвертных нотах (от медленного к быстрому)
    struct RateDiv { const char* name; double quarterNotes; };
    inline constexpr RateDiv rateTable[] =
    {
        { "1/1",   4.0 },      { "1/2",   2.0 },      { "1/4D",  1.5 },
        { "1/4",   1.0 },      { "1/8D",  0.75 },     { "1/4T",  2.0 / 3.0 },
        { "1/8",   0.5 },      { "1/16D", 0.375 },    { "1/8T",  1.0 / 3.0 },
        { "1/16",  0.25 },     { "1/32D", 0.1875 },   { "1/16T", 1.0 / 6.0 },
        { "1/32",  0.125 },    { "1/32T", 1.0 / 12.0 },{ "1/64",  0.0625 }
    };
    inline constexpr int numRates = (int) (sizeof (rateTable) / sizeof (rateTable[0]));
    inline constexpr int defaultRate = 9; // 1/16

    struct ScaleDef { const char* name; int count; int notes[12]; };
    inline constexpr ScaleDef scales[] =
    {
        { "Major",       7,  { 0, 2, 4, 5, 7, 9, 11 } },
        { "Minor",       7,  { 0, 2, 3, 5, 7, 8, 10 } },
        { "Pentatonic",  5,  { 0, 2, 4, 7, 9 } },
        { "Dorian",      7,  { 0, 2, 3, 5, 7, 9, 10 } },
        { "Phrygian",    7,  { 0, 1, 3, 5, 7, 8, 10 } },
        { "Harm. Minor", 7,  { 0, 2, 3, 5, 7, 8, 11 } },
        { "Whole Tone",  6,  { 0, 2, 4, 6, 8, 10 } },
        { "Fifths",      3,  { 0, 7, 12 } },
        { "Chromatic",   12, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } }
    };
    inline constexpr int numScales = (int) (sizeof (scales) / sizeof (scales[0]));

    inline constexpr const char* keyNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    // Тоника -> транспонирование в полутонах (исходник считается в C), в диапазоне -5..+6
    inline int keyToSemis (int key) { return key <= 6 ? key : key - 12; }
}
