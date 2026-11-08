#pragma once

struct VortexPreset
{
    const char* name;
    int   rate;     // индекс в rateTable
    float size;     // % от шага сетки
    float scan;     // -1..1
    float harmony;  // 0..1
    float chaos;    // 0..1
    float mix;      // 0..1
    int   key;      // 0..11
    int   scale;    // индекс в scales
    int   buffer;   // 0=1 bar, 1=2 bars, 2=4 bars
};

inline constexpr VortexPreset vortexPresets[] =
{
    { "Init Portal",      9, 100,  0.00f, 0.00f, 0.00f, 0.65f, 0, 0, 1 },
    { "Glass Arp",        9,  80,  0.15f, 0.70f, 0.25f, 0.80f, 0, 2, 1 },
    { "Tape Stutter",     6, 140,  0.00f, 0.00f, 0.15f, 0.85f, 0, 0, 0 },
    { "Cathedral Freeze", 6, 320,  0.04f, 0.60f, 0.20f, 0.90f, 0, 3, 2 },
    { "Broken Piano",     7,  90, -0.20f, 0.50f, 0.45f, 0.85f, 0, 0, 1 },
    { "Rhythm Slicer",   12,  60,  0.40f, 0.00f, 0.30f, 1.00f, 0, 0, 0 },
    { "Ghost Choir",      3, 350,  0.02f, 0.90f, 0.35f, 0.75f, 0, 3, 2 },
    { "Reverse Bloom",    6, 200, -0.50f, 0.30f, 0.30f, 0.80f, 0, 1, 1 },
    { "Minor Rain",      11,  70,  0.10f, 0.80f, 0.40f, 0.75f, 0, 1, 1 },
    { "Pulse Garden",     8, 150,  0.25f, 0.60f, 0.20f, 0.70f, 0, 2, 2 },
    { "Warp Drive",      13,  50,  0.80f, 0.20f, 0.60f, 0.90f, 0, 6, 0 },
    { "Vinyl Dust",      14,  40,  0.05f, 0.00f, 0.80f, 0.50f, 0, 0, 0 },
    { "Octave Ladder",    9, 110,  0.00f, 1.00f, 0.10f, 0.80f, 0, 7, 0 },
    { "Dark Matter",      4, 280, -0.10f, 0.70f, 0.50f, 0.85f, 9, 4, 2 },
    { "Sunrise Loop",    10, 120,  0.30f, 0.50f, 0.15f, 0.70f, 7, 0, 1 },
    { "Event Horizon",   12, 200,  0.60f, 0.90f, 0.90f, 1.00f, 0, 5, 2 }
};
inline constexpr int numPresets = (int) (sizeof (vortexPresets) / sizeof (vortexPresets[0]));
