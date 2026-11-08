#pragma once
#include <JuceHeader.h>
#include "Params.h"
#include <array>
#include <vector>
#include <cmath>
#include <limits>

// Событие гранулы для визуализации (аудио-поток -> UI)
struct GrainEvent
{
    float pos = 0.f;       // позиция чтения в петле, 0..1
    float semis = 0.f;     // питч в полутонах
    float pan = 0.f;       // -1..1
    float lengthMs = 0.f;
    bool  reverse = false;
};

struct BlockParams
{
    float sizePct = 120.f, scan = 0.f, harmony = 0.f, chaos = 0.f, mix = 0.7f;
    int   scale = 0, key = 0;
    bool  freeze = false;
};

class GranularEngine
{
public:
    static constexpr int maxGrains = 64;

    void prepare (double sr)
    {
        sampleRate = sr;
        maxLen = (int) (sr * 34.0);
        for (auto& ch : ring) ch.assign ((size_t) maxLen + 8, 0.f);
        for (auto& g : grains) g.active = false;
        lastStep = std::numeric_limits<long long>::min();
        lastPhase = 0.0; lastWPos = 0.0; scanOffset = 0.0; frozenAnchor = 0.0;
        wasFrozen = false;
        loopLen = 1024.0;
        mixSmooth.reset (sr, 0.02);
        mixSmooth.setCurrentAndTargetValue (0.7f);
    }

    bool popEvent (GrainEvent& e)
    {
        if (fifo.getNumReady() < 1) return false;
        int s1, n1, s2, n2;
        fifo.prepareToRead (1, s1, n1, s2, n2);
        e = events[(size_t) s1];
        fifo.finishedRead (1);
        return true;
    }

    // ppqStart: позиция в четвертях на начале блока. Буфер привязан к сетке такта.
    void process (juce::AudioBuffer<float>& buffer, double ppqStart, double bpm,
                  int bars, double stepQN, const BlockParams& p)
    {
        const int n = buffer.getNumSamples();
        if (n == 0 || maxLen == 0) return;

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        const double spQN      = sampleRate * 60.0 / juce::jmax (20.0, bpm);
        const double cycleQN   = (double) bars * 4.0;
        loopLen                = std::floor (juce::jlimit (256.0, (double) maxLen, cycleQN * spQN));
        const double stepSamp  = stepQN * spQN;
        const long long stepsPerCycle = juce::jmax (1LL, (long long) std::llround (cycleQN / stepQN));
        const float gainComp   = 1.f / std::sqrt (juce::jmax (1.f, p.sizePct / 100.f));
        const float scanSpeed  = p.scan * std::abs (p.scan) * 2.f;

        mixSmooth.setTargetValue (p.mix);
        if (p.freeze && ! wasFrozen) frozenAnchor = lastWPos;
        wasFrozen = p.freeze;

        const int loopI = (int) loopLen;

        for (int i = 0; i < n; ++i)
        {
            const double ppqNow = ppqStart + (double) i / spQN;
            double cyc = std::fmod (ppqNow, cycleQN);
            if (cyc < 0.0) cyc += cycleQN;
            const double phase = cyc / cycleQN;
            const double wPos  = phase * loopLen;

            if (phase < lastPhase - 0.5) scanOffset = 0.0; // новый цикл -> скан начинается заново (повторяемо)
            lastPhase = phase;
            lastWPos  = wPos;

            const long long stepIdx = (long long) std::floor (ppqNow / stepQN + 1e-7);
            if (stepIdx != lastStep)
            {
                lastStep = stepIdx;
                trigger (stepIdx, stepsPerCycle, stepSamp, wPos, p);
            }

            const float inL = outL[i];
            const float inR = outR != nullptr ? outR[i] : inL;

            float wl = 0.f, wr = 0.f;
            for (auto& g : grains)
            {
                if (! g.active) continue;

                double rp = g.readPos;
                if (rp >= loopLen || rp < 0.0) rp = wrap (rp);
                int i0 = (int) rp;
                if (i0 >= loopI) i0 = loopI - 1;
                const float fr = (float) (rp - (double) i0);
                int i1 = i0 + 1; if (i1 >= loopI) i1 = 0;

                const float sL = ring[0][(size_t) i0] + fr * (ring[0][(size_t) i1] - ring[0][(size_t) i0]);
                const float sR = ring[1][(size_t) i0] + fr * (ring[1][(size_t) i1] - ring[1][(size_t) i0]);

                const float t   = ((float) g.age + 0.5f) / (float) g.len;
                const float env = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * t);

                wl += sL * env * g.gl;
                wr += sR * env * g.gr;

                g.readPos += (double) g.inc;
                if (g.readPos >= loopLen) g.readPos -= loopLen;
                else if (g.readPos < 0.0) g.readPos += loopLen;

                if (++g.age >= g.len) g.active = false;
            }

            if (! p.freeze)
            {
                const int w = juce::jlimit (0, loopI - 1, (int) wPos);
                ring[0][(size_t) w] = inL;
                ring[1][(size_t) w] = inR;
            }

            scanOffset += (double) scanSpeed;
            if (scanOffset > loopLen) scanOffset -= loopLen;
            else if (scanOffset < -loopLen) scanOffset += loopLen;

            const float m = mixSmooth.getNextValue();
            wl = std::tanh (wl * gainComp);
            wr = std::tanh (wr * gainComp);

            outL[i] = inL * (1.f - m) + wl * m;
            if (outR != nullptr) outR[i] = inR * (1.f - m) + wr * m;
        }
    }

private:
    struct Grain
    {
        double readPos = 0.0;
        float  inc = 1.f, gl = 0.7f, gr = 0.7f;
        int    len = 1, age = 0;
        bool   active = false;
    };

    double wrap (double x) const
    {
        x = std::fmod (x, loopLen);
        if (x < 0.0) x += loopLen;
        return x;
    }

    void trigger (long long stepIdx, long long spc, double stepSamples, double wPos, const BlockParams& p)
    {
        const long long inCycle = ((stepIdx % spc) + spc) % spc;
        // Сид зависит только от номера шага в цикле -> Chaos повторяется каждый цикл
        juce::Random r ((juce::int64) ((inCycle + 1) * 2654435761LL) ^ 0x5bd1e995LL);

        spawn (r, stepSamples, wPos, p, 1.f);
        if (p.chaos > 0.05f && r.nextFloat() < p.chaos * 0.35f)
            spawn (r, stepSamples, wPos, p, 0.7f);
    }

    void spawn (juce::Random& r, double stepSamples, double wPos, const BlockParams& p, float level)
    {
        const double len = juce::jlimit (48.0, sampleRate * 3.0, (double) p.sizePct * 0.01 * stepSamples);

        float semis = 0.f;
        if (p.harmony > 0.001f && r.nextFloat() < juce::jmin (1.f, p.harmony * 1.2f))
        {
            const auto& sc = VortexParams::scales[juce::jlimit (0, VortexParams::numScales - 1, p.scale)];
            const int deg = r.nextInt (sc.count);
            int oct = 0;
            if (p.harmony > 0.25f) oct = r.nextInt (p.harmony > 0.7f ? 4 : 3) - 1; // -1..+1 или -1..+2
            semis = (float) (sc.notes[deg] + 12 * oct + VortexParams::keyToSemis (p.key));
        }

        const double jitter = (double) p.chaos * (double) (r.nextFloat() * 2.f - 1.f) * 0.25 * loopLen;
        const bool reverse  = r.nextFloat() < p.chaos * 0.5f;
        const float pan     = p.chaos * (r.nextFloat() * 2.f - 1.f);
        const float inc     = std::pow (2.f, semis / 12.f);

        const double anchor = p.freeze ? frozenAnchor : wPos;
        double start = anchor + scanOffset + jitter;
        if (! reverse && ! p.freeze) start -= len * (double) inc; // читаем уже записанный звук, а не гонимся за головкой записи
        start = wrap (start);

        Grain* g = nullptr;
        int oldest = -1;
        for (auto& gr : grains)
        {
            if (! gr.active) { g = &gr; break; }
            if (gr.age > oldest) { oldest = gr.age; g = &gr; }
        }
        if (g == nullptr) return;

        g->readPos = start;
        g->inc     = reverse ? -inc : inc;
        g->len     = (int) len;
        g->age     = 0;
        const float a = (pan + 1.f) * juce::MathConstants<float>::pi * 0.25f;
        g->gl = std::cos (a) * 1.4142f * level;
        g->gr = std::sin (a) * 1.4142f * level;
        g->active = true;

        if (fifo.getFreeSpace() >= 1)
        {
            int s1, n1, s2, n2;
            fifo.prepareToWrite (1, s1, n1, s2, n2);
            events[(size_t) s1] = { (float) (start / loopLen), semis, pan,
                                    (float) (len / sampleRate * 1000.0), reverse };
            fifo.finishedWrite (1);
        }
    }

    double sampleRate = 44100.0;
    int    maxLen = 0;
    double loopLen = 1024.0;
    std::array<std::vector<float>, 2> ring;
    std::array<Grain, maxGrains> grains;

    long long lastStep = std::numeric_limits<long long>::min();
    double lastPhase = 0.0, lastWPos = 0.0, scanOffset = 0.0, frozenAnchor = 0.0;
    bool   wasFrozen = false;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmooth;

    juce::AbstractFifo fifo { 512 };
    std::array<GrainEvent, 512> events;
};
