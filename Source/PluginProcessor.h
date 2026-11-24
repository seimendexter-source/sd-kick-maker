#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "KickEngine.h"

struct KickPreset
{
    const char* name;
    float punch, body, length, drive, rumble;
};

class SDKickMakerProcessor : public juce::AudioProcessor
{
public:
    SDKickMakerProcessor();
    ~SDKickMakerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SD KICK MAKER"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- für die Oberfläche ---
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    KickParams currentParams() const;
    void preview() { previewRequested = true; }
    bool loadSampleFile (const juce::File& f);
    void clearSample();
    juce::String getSampleInfo() const;
    std::shared_ptr<const KickSample> getSamplePtr() const { return engine.getSample(); }

    static const std::vector<KickPreset>& presets();
    void loadPreset (int index);

    void toggleAB();
    bool isOnB() const { return abIsB; }

    float getAndResetPeak() { return peak.exchange (0.0f); }
    double getHostBpm() const { return hostBpm.load(); }

    bool exportWav (const juce::File& f);

    static const juce::StringArray keyNames;

private:
    KickEngine engine;
    std::atomic<bool> previewRequested { false };
    std::atomic<float> peak { 0.0f };
    std::atomic<double> hostBpm { 160.0 };
    juce::String samplePath;

    juce::ValueTree abSlot[2];
    bool abIsB = false;

    juce::AudioFormatManager formats;
    std::vector<float> monoBuf;
    int engineBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SDKickMakerProcessor)
};
