#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace VortexParams;

juce::AudioProcessorValueTreeState::ParameterLayout VortexAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray rates;  for (auto& r : rateTable) rates.add (r.name);
    StringArray keys;   for (auto* k : keyNames)  keys.add (k);
    StringArray scs;    for (auto& s : scales)    scs.add (s.name);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pRate, 1 }, "Rate", rates, defaultRate));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { pSize, 1 }, "Size",
                    NormalisableRange<float> (10.f, 400.f, 0.1f, 0.5f), 120.f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { pScan, 1 }, "Scan",
                    NormalisableRange<float> (-1.f, 1.f, 0.001f), 0.f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { pHarmony, 1 }, "Harmony",
                    NormalisableRange<float> (0.f, 1.f, 0.001f), 0.f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { pChaos, 1 }, "Chaos",
                    NormalisableRange<float> (0.f, 1.f, 0.001f), 0.15f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { pMix, 1 }, "Mix",
                    NormalisableRange<float> (0.f, 1.f, 0.001f), 0.7f));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { pFreeze, 1 }, "Freeze", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pKey, 1 }, "Key", keys, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pScale, 1 }, "Scale", scs, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pBuffer, 1 }, "Buffer",
                    StringArray { "1 Bar", "2 Bars", "4 Bars" }, 1));
    return layout;
}

VortexAudioProcessor::VortexAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VORTEX", createLayout())
{
    rateP    = apvts.getRawParameterValue (pRate);
    sizeP    = apvts.getRawParameterValue (pSize);
    scanP    = apvts.getRawParameterValue (pScan);
    harmonyP = apvts.getRawParameterValue (pHarmony);
    chaosP   = apvts.getRawParameterValue (pChaos);
    mixP     = apvts.getRawParameterValue (pMix);
    freezeP  = apvts.getRawParameterValue (pFreeze);
    keyP     = apvts.getRawParameterValue (pKey);
    scaleP   = apvts.getRawParameterValue (pScale);
    bufferP  = apvts.getRawParameterValue (pBuffer);
}

void VortexAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    fallbackPpq = 0.0;
}

bool VortexAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return out == l.getMainInputChannelSet();
}

void VortexAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    // --- тактовая синхронизация с хостом ---
    double bpm = 120.0, ppq = 0.0;
    bool playing = false, havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())         bpm = *b;
            if (auto q = pos->getPpqPosition()) { ppq = *q; havePpq = true; }
            playing = pos->getIsPlaying();
        }
    bpm = juce::jmax (20.0, bpm);

    const double spQN = getSampleRate() * 60.0 / bpm;
    double ppqUse;
    if (playing && havePpq) { ppqUse = ppq;         fallbackPpq = ppq + (double) n / spQN; }
    else                    { ppqUse = fallbackPpq; fallbackPpq += (double) n / spQN; }

    BlockParams bp;
    bp.sizePct = sizeP->load();
    bp.scan    = scanP->load();
    bp.harmony = harmonyP->load();
    bp.chaos   = chaosP->load();
    bp.mix     = mixP->load();
    bp.freeze  = freezeP->load() > 0.5f;
    bp.key     = juce::jlimit (0, 11, (int) std::round (keyP->load()));
    bp.scale   = juce::jlimit (0, numScales - 1, (int) std::round (scaleP->load()));

    const int rateIdx = juce::jlimit (0, numRates - 1, (int) std::round (rateP->load()));
    const int bars = getBars();

    engine.process (buffer, ppqUse, bpm, bars, rateTable[rateIdx].quarterNotes, bp);

    // --- данные для UI ---
    const double cycleQN = (double) bars * 4.0;
    double ph = std::fmod (ppqUse + (double) n / spQN, cycleQN) / cycleQN;
    if (ph < 0.0) ph += 1.0;
    visPhase.store ((float) ph);
    visBpm.store ((float) bpm);

    float lvl = 0.f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        lvl = juce::jmax (lvl, buffer.getMagnitude (ch, 0, n));
    visLevel.store (visLevel.load() * 0.8f + lvl * 0.2f);
}

void VortexAudioProcessor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, numPresets - 1, index);
    currentProgram = index;
    const auto& p = vortexPresets[index];

    auto setPlain = [this] (const char* id, float v)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };
    setPlain (pRate,    (float) p.rate);
    setPlain (pSize,    p.size);
    setPlain (pScan,    p.scan);
    setPlain (pHarmony, p.harmony);
    setPlain (pChaos,   p.chaos);
    setPlain (pMix,     p.mix);
    setPlain (pKey,     (float) p.key);
    setPlain (pScale,   (float) p.scale);
    setPlain (pBuffer,  (float) p.buffer);
}

const juce::String VortexAudioProcessor::getProgramName (int index)
{
    return vortexPresets[juce::jlimit (0, numPresets - 1, index)].name;
}

void VortexAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void VortexAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, numPresets - 1, (int) vt.getProperty ("program", 0));
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* VortexAudioProcessor::createEditor()
{
    return new VortexAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VortexAudioProcessor();
}
