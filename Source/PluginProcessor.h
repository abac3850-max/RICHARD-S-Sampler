#pragma once
#include <JuceHeader.h>
#include <array>
#include <memory>

class RASamplerAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int slotCount = 8;
    static constexpr int maxVoices = 64;

    struct SlotInfo
    {
        juce::String name { "Empty" };
        int rootNote = 60;
        int lowNote = 60;
        int highNote = 60;
        int lowVelocity = 1;
        int highVelocity = 127;
        bool loopEnabled = false;
        bool loaded = false;
        std::shared_ptr<juce::AudioBuffer<float>> audio;
        double sourceSampleRate = 44100.0;
        float start = 0.0f, end = 1.0f;
        float loopStart = 0.0f, loopEnd = 1.0f;
        float fadeIn = 0.0f, fadeOut = 0.0f;
    };

    RASamplerAudioProcessor();
    ~RASamplerAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    bool loadSample(int slot, const juce::File& file, juce::String& error);
    const SlotInfo& getSlot(int slot) const;
    void setSlotLoop(int slot, bool shouldLoop);
    void setSlotZone(int slot, int root, int low, int high);
    void setSlotGain(int slot, float gain);
    float getSlotGain(int slot) const;
    void setSlotEdit(int slot, float start, float end, float loopStart, float loopEnd, float fadeIn, float fadeOut);
    void setSkinIndex(int index) { skinIndex = juce::jlimit(0, 9, index); }
    int getSkinIndex() const { return skinIndex; }

private:
    class SlotSound;
    class SlotVoice;
    void rebuildSounds();

    juce::AudioFormatManager formatManager;
    juce::Synthesiser synth;
    std::array<SlotInfo, slotCount> slots;
    std::array<float, slotCount> slotGains;
    int skinIndex = 0;
    juce::CriticalSection slotLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RASamplerAudioProcessor)
};
