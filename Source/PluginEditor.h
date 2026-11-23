#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class RASamplerAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RASamplerAudioProcessorEditor(RASamplerAudioProcessor&);
    ~RASamplerAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Skin { const char* name; juce::uint32 background, panel, panelAlt, accent, accentDark, text, muted, border; };
    class WaveformView final : public juce::Component
    {
    public:
        explicit WaveformView(RASamplerAudioProcessorEditor& e) : owner(e) {}
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
    private:
        enum Handle { none, start, end, fadeIn, fadeOut, loopStart, loopEnd };
        Handle active = none;
        RASamplerAudioProcessorEditor& owner;
    };
    class GreenLook final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    };
    static const std::array<Skin, 10>& getSkins();
    void applySkin();
    void timerCallback() override;
    void selectSlot(int slot);
    void loadSelectedSample();
    void loadFactoryKit(int presetIndex);
    void updateEditFromWaveform();

    RASamplerAudioProcessor& processor;
    int selectedSlot = 0, selectedSkin = 0;
    juce::Label title, slotTitle, waveformLabel;
    juce::TextButton loadButton { "LOAD SAMPLE" }, loopButton { "LOOP: OFF" }, reverseButton { "REVERSE" };
    juce::ComboBox skinBox, factoryBox;
    juce::Slider gainSlider, panSlider, pitchSlider, fadeInSlider, fadeOutSlider;
    juce::Label gainLabel, panLabel, pitchLabel, fadeInLabel, fadeOutLabel;
    std::array<std::unique_ptr<juce::TextButton>, RASamplerAudioProcessor::slotCount> slotButtons;
    std::unique_ptr<juce::FileChooser> chooser;
    WaveformView waveform { *this };
    GreenLook look;
    juce::StringArray presetNames { "RA Core Drum Kit", "RA Hip-Hop Kit", "RA Electronic Kit", "RA Afro Percussion Kit", "RA Minimal Kit" };
    juce::File factoryPresetDirectory;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RASamplerAudioProcessorEditor)
};
