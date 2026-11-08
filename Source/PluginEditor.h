#pragma once
#include <JuceHeader.h>
#include <vector>
#include "PluginProcessor.h"

namespace Theme
{
    inline const juce::Colour bg      { 0xff05040c };
    inline const juce::Colour cyan    { 0xff35f2ff };
    inline const juce::Colour violet  { 0xff8a5cff };
    inline const juce::Colour magenta { 0xffff3df2 };
    inline const juce::Colour amber   { 0xffff8a5c };
    inline const juce::Colour ink     { 0xffeae6ff };
    inline const juce::Colour dim     { 0xff8f86c9 };
}

//==============================================================================
class VortexLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VortexLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;

    float pulse = 0.f; // 0..1, пульс от выходного уровня
};

//==============================================================================
// Центральный "портал": кольцо-такт, вихрь, частицы-гранулы
class PortalComponent : public juce::Component, private juce::Timer
{
public:
    explicit PortalComponent (VortexAudioProcessor&);
    ~PortalComponent() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void resized() override;
    float getPulse() const { return juce::jlimit (0.f, 1.f, level * 0.7f + kick * 0.35f); }

private:
    struct Particle { float a0, age, life, size, hue, dir, swirl; bool reverse; };
    struct Flash    { float ang, life, hue; };
    struct Star     { float ang, rad, spd, size, phase, tw, bright; bool warm; };

    void timerCallback() override;
    void spawn (const GrainEvent&);

    VortexAudioProcessor& proc;
    juce::Image trailImage;
    std::vector<Particle> particles;
    std::vector<Flash> flashes;
    std::vector<Star> stars;

    juce::Point<float> centre;
    float ringR = 190.f;
    double displayPhase = 0.0, lastMs = 0.0;
    float bpm = 120.f, level = 0.f, kick = 0.f;
    int bars = 2;
};

//==============================================================================
class VortexAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VortexAudioProcessorEditor (VortexAudioProcessor&);
    ~VortexAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void setupKnob (juce::Slider&, const juce::String& id, const juce::String& name,
                    juce::Colour accent, std::unique_ptr<SliderAtt>&);
    void setupCombo (juce::ComboBox&, const char* id, std::unique_ptr<ComboAtt>&);
    void stepPreset (int delta);
    void refreshPresetName();

    VortexAudioProcessor& proc;
    VortexLookAndFeel laf;
    juce::Image bgImage;

    std::unique_ptr<PortalComponent> portal;

    juce::Slider rateK, sizeK, scanK, harmK, chaosK, mixK;
    juce::ComboBox keyBox, scaleBox, bufBox;
    juce::TextButton freezeBtn { "FREEZE" }, prevBtn { "<" }, nextBtn { ">" }, presetBtn;

    std::unique_ptr<SliderAtt> rateA, sizeA, scanA, harmA, chaosA, mixA;
    std::unique_ptr<ComboAtt>  keyA, scaleA, bufA;
    std::unique_ptr<ButtonAtt> freezeA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VortexAudioProcessorEditor)
};
