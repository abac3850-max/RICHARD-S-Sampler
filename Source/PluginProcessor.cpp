#include "PluginProcessor.h"
#include "PluginEditor.h"

class RASamplerAudioProcessor::SlotSound final : public juce::SynthesiserSound
{
public:
    SlotSound(int slotIndex, const SlotInfo& info)
        : slot(slotIndex), sample(info.audio), sourceRate(info.sourceSampleRate), root(info.rootNote),
          low(info.lowNote), high(info.highNote), loop(info.loopEnabled), gain(1.0f),
          start(info.start), end(info.end), loopStart(info.loopStart), loopEnd(info.loopEnd),
          fadeIn(info.fadeIn), fadeOut(info.fadeOut) {}

    bool appliesToNote(int note) override { return note >= low && note <= high; }
    bool appliesToChannel(int) override { return true; }

    int slot = 0;
    std::shared_ptr<juce::AudioBuffer<float>> sample;
    double sourceRate = 44100.0;
    int root = 60, low = 60, high = 60;
    bool loop = false;
    float gain = 1.0f;
    float start = 0.0f, end = 1.0f, loopStart = 0.0f, loopEnd = 1.0f;
    float fadeIn = 0.0f, fadeOut = 0.0f;
};

class RASamplerAudioProcessor::SlotVoice final : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    { return dynamic_cast<SlotSound*>(sound) != nullptr; }

    void startNote(int midiNote, float velocity, juce::SynthesiserSound* sound, int) override
    {
        auto* s = dynamic_cast<SlotSound*>(sound);
        if (s == nullptr || s->sample == nullptr || s->sample->getNumSamples() == 0)
        { clearCurrentNote(); return; }

        currentSample = s->sample;
        sourceRate = s->sourceRate;
        loop = s->loop;
        slotIndex = s->slot;
        slotGain = s->gain;
        regionStart = juce::jlimit(0, currentSample->getNumSamples() - 1, (int) (s->start * currentSample->getNumSamples()));
        regionEnd = juce::jlimit(regionStart + 1, currentSample->getNumSamples(), (int) (s->end * currentSample->getNumSamples()));
        loopStartSample = juce::jlimit(regionStart, regionEnd - 1, (int) (s->loopStart * currentSample->getNumSamples()));
        loopEndSample = juce::jlimit(loopStartSample + 1, regionEnd, (int) (s->loopEnd * currentSample->getNumSamples()));
        fadeInSamples = (int) ((s->fadeIn) * (regionEnd - regionStart));
        fadeOutSamples = (int) ((s->fadeOut) * (regionEnd - regionStart));
        position = (double) regionStart;
        level = velocity;
        pitchRatio = (sourceRate / juce::jmax(1.0, getSampleRate()))
                   * std::pow(2.0, (midiNote - s->root) / 12.0);
        envelope.reset(getSampleRate(), 0.008);
        envelope.setParameters({ 0.008f, 0.08f, 1.0f, 0.12f });
        envelope.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff) envelope.noteOff();
        else { envelope.reset(); clearCurrentNote(); currentSample.reset(); }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& output, int start, int count) override
    {
        if (! isVoiceActive() || currentSample == nullptr) return;
        const int sourceChannels = currentSample->getNumChannels();
        const int sourceLength = currentSample->getNumSamples();
        if (sourceChannels == 0 || sourceLength == 0) { clearCurrentNote(); return; }

        for (int i = 0; i < count; ++i)
        {
            if (position >= regionEnd)
            {
                if (loop) position = loopStartSample + std::fmod(position - loopStartSample, (double) juce::jmax(1, loopEndSample - loopStartSample));
                else { clearCurrentNote(); currentSample.reset(); break; }
            }

            const int indexA = juce::jlimit(0, sourceLength - 1, (int) position);
            const int indexB = (loop && indexA + 1 >= loopEndSample) ? loopStartSample : juce::jmin(indexA + 1, regionEnd - 1);
            const float fraction = (float) (position - indexA);
            float env = envelope.getNextSample();
            const int relative = juce::jmax(0, indexA - regionStart);
            if (fadeInSamples > 0 && relative < fadeInSamples)
                env *= (float) relative / (float) fadeInSamples;
            const int remaining = regionEnd - indexA;
            if (fadeOutSamples > 0 && remaining <= fadeOutSamples)
                env *= juce::jlimit(0.0f, 1.0f, (float) remaining / (float) fadeOutSamples);
            for (int ch = 0; ch < output.getNumChannels(); ++ch)
            {
                const int srcCh = juce::jmin(ch, sourceChannels - 1);
                const float a = currentSample->getSample(srcCh, indexA);
                const float b = currentSample->getSample(srcCh, indexB);
                output.addSample(ch, start + i, (a + (b - a) * fraction) * level * slotGain * env);
            }
            position += pitchRatio;
            if (! envelope.isActive() && envelope.getNextSample() == 0.0f && ! loop)
            { clearCurrentNote(); currentSample.reset(); break; }
        }
    }

private:
    std::shared_ptr<juce::AudioBuffer<float>> currentSample;
    juce::ADSR envelope;
    double sourceRate = 44100.0, position = 0.0, pitchRatio = 1.0;
    float level = 1.0f, slotGain = 1.0f;
    int slotIndex = 0;
    int regionStart = 0, regionEnd = 1, loopStartSample = 0, loopEndSample = 1;
    int fadeInSamples = 0, fadeOutSamples = 0;
    bool loop = false;
};

RASamplerAudioProcessor::RASamplerAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
    slotGains.fill(1.0f);
    for (int i = 0; i < maxVoices; ++i) synth.addVoice(new SlotVoice());
}

void RASamplerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    synth.setNoteStealingEnabled(true);
    juce::ignoreUnused(samplesPerBlock);
}

void RASamplerAudioProcessor::releaseResources() {}

bool RASamplerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void RASamplerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}

void RASamplerAudioProcessor::rebuildSounds()
{
    const juce::ScopedLock lock(slotLock);
    synth.clearSounds();
    for (int i = 0; i < slotCount; ++i)
    {
        if (slots[(size_t)i].loaded)
        {
            auto* sound = new SlotSound(i, slots[(size_t)i]);
            sound->gain = slotGains[(size_t)i];
            synth.addSound(sound);
        }
    }
}

bool RASamplerAudioProcessor::loadSample(int slot, const juce::File& file, juce::String& error)
{
    if (! juce::isPositiveAndBelow(slot, slotCount)) { error = "Invalid sample slot."; return false; }
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr) { error = "This audio format could not be decoded. Try WAV, AIFF, FLAC or another supported format."; return false; }
    if (reader->lengthInSamples <= 0 || reader->lengthInSamples > (juce::int64) reader->sampleRate * 600.0)
    { error = "The file is empty or longer than the current 10-minute safety limit."; return false; }

    auto audio = std::make_shared<juce::AudioBuffer<float>>((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read(audio.get(), 0, (int) reader->lengthInSamples, 0, true, true);
    {
        const juce::ScopedLock lock(slotLock);
        auto& s = slots[(size_t)slot];
        s.name = file.getFileName(); s.audio = std::move(audio); s.sourceSampleRate = reader->sampleRate;
        s.loaded = true; s.rootNote = 60 + slot; s.lowNote = s.rootNote; s.highNote = s.rootNote;
    }
    rebuildSounds();
    error.clear();
    return true;
}

const RASamplerAudioProcessor::SlotInfo& RASamplerAudioProcessor::getSlot(int slot) const
{
    jassert(juce::isPositiveAndBelow(slot, slotCount));
    return slots[(size_t) juce::jlimit(0, slotCount - 1, slot)];
}

void RASamplerAudioProcessor::setSlotLoop(int slot, bool shouldLoop)
{
    if (! juce::isPositiveAndBelow(slot, slotCount)) return;
    { const juce::ScopedLock lock(slotLock); slots[(size_t)slot].loopEnabled = shouldLoop; }
    rebuildSounds();
}

void RASamplerAudioProcessor::setSlotZone(int slot, int root, int low, int high)
{
    if (! juce::isPositiveAndBelow(slot, slotCount)) return;
    { const juce::ScopedLock lock(slotLock); auto& s = slots[(size_t)slot]; s.rootNote = juce::jlimit(0,127,root); s.lowNote = juce::jlimit(0,127,low); s.highNote = juce::jlimit(s.lowNote,127,high); }
    rebuildSounds();
}

void RASamplerAudioProcessor::setSlotGain(int slot, float gain)
{
    if (! juce::isPositiveAndBelow(slot, slotCount)) return;
    slotGains[(size_t)slot] = juce::jlimit(0.0f, 2.0f, gain);
    rebuildSounds();
}
float RASamplerAudioProcessor::getSlotGain(int slot) const
{ return juce::isPositiveAndBelow(slot, slotCount) ? slotGains[(size_t)slot] : 1.0f; }

void RASamplerAudioProcessor::setSlotEdit(int slot, float start, float end, float loopStart, float loopEnd, float fadeIn, float fadeOut)
{
    if (! juce::isPositiveAndBelow(slot, slotCount)) return;
    {
        const juce::ScopedLock lock(slotLock);
        auto& s = slots[(size_t) slot];
        s.start = juce::jlimit(0.0f, 0.98f, start);
        s.end = juce::jlimit(s.start + 0.01f, 1.0f, end);
        s.loopStart = juce::jlimit(s.start, s.end - 0.01f, loopStart);
        s.loopEnd = juce::jlimit(s.loopStart + 0.01f, s.end, loopEnd);
        s.fadeIn = juce::jlimit(0.0f, 1.0f, fadeIn);
        s.fadeOut = juce::jlimit(0.0f, 1.0f, fadeOut);
    }
    rebuildSounds();
}

void RASamplerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree state("RASamplerState");
    state.setProperty("skinIndex", skinIndex, nullptr);
    for (int i = 0; i < slotCount; ++i)
    {
        const auto& s = slots[(size_t)i];
        juce::ValueTree child("Slot");
        child.setProperty("index", i, nullptr); child.setProperty("file", s.audio != nullptr ? s.name : juce::String(), nullptr);
        child.setProperty("root", s.rootNote, nullptr); child.setProperty("low", s.lowNote, nullptr);
        child.setProperty("high", s.highNote, nullptr); child.setProperty("loop", s.loopEnabled, nullptr);
        child.setProperty("start", s.start, nullptr); child.setProperty("end", s.end, nullptr);
        child.setProperty("loopStart", s.loopStart, nullptr); child.setProperty("loopEnd", s.loopEnd, nullptr);
        child.setProperty("fadeIn", s.fadeIn, nullptr); child.setProperty("fadeOut", s.fadeOut, nullptr);
        state.addChild(child, -1, nullptr);
    }
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void RASamplerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName("RASamplerState")) return;
    skinIndex = juce::jlimit(0, 9, xml->getIntAttribute("skinIndex", 0));
    juce::XmlDocument doc(*xml);
    for (auto* child : xml->getChildIterator())
    {
        const int i = child->getIntAttribute("index", -1);
        if (! juce::isPositiveAndBelow(i, slotCount)) continue;
        auto& s = slots[(size_t)i];
        s.rootNote = child->getIntAttribute("root", 60 + i);
        s.lowNote = child->getIntAttribute("low", s.rootNote);
        s.highNote = child->getIntAttribute("high", s.rootNote);
        s.loopEnabled = child->getBoolAttribute("loop", false);
        s.start = (float) child->getDoubleAttribute("start", 0.0);
        s.end = (float) child->getDoubleAttribute("end", 1.0);
        s.loopStart = (float) child->getDoubleAttribute("loopStart", 0.0);
        s.loopEnd = (float) child->getDoubleAttribute("loopEnd", 1.0);
        s.fadeIn = (float) child->getDoubleAttribute("fadeIn", 0.0);
        s.fadeOut = (float) child->getDoubleAttribute("fadeOut", 0.0);
        // Sample audio is deliberately not embedded in this first prototype.
        // The next persistence milestone will store portable sample references and copy/embed options.
    }
    rebuildSounds();
}

juce::AudioProcessorEditor* RASamplerAudioProcessor::createEditor()
{ return new RASamplerAudioProcessorEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{ return new RASamplerAudioProcessor(); }
