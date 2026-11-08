#pragma once
#include <JuceHeader.h>
#include "Params.h"
#include "Presets.h"
#include "GranularEngine.h"

class VortexAudioProcessor : public juce::AudioProcessor
{
public:
    VortexAudioProcessor();
    ~VortexAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VORTEX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return numPresets; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    int getBars() const { return 1 << juce::jlimit (0, 2, (int) std::round (bufferP->load())); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    GranularEngine engine;

    // данные для визуализации
    std::atomic<float> visPhase { 0.f }, visBpm { 120.f }, visLevel { 0.f };

private:
    std::atomic<float> *rateP = nullptr, *sizeP = nullptr, *scanP = nullptr, *harmonyP = nullptr,
                       *chaosP = nullptr, *mixP = nullptr, *freezeP = nullptr, *keyP = nullptr,
                       *scaleP = nullptr, *bufferP = nullptr;

    double fallbackPpq = 0.0;   // внутренние часы, когда транспорт остановлен
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VortexAudioProcessor)
};
