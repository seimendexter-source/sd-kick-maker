#include "PluginProcessor.h"
#include "PluginEditor.h"

const juce::StringArray SDKickMakerProcessor::keyNames { "C", "C#", "D", "D#", "E", "F",
                                                         "F#", "G", "G#", "A", "A#", "B" };

const std::vector<KickPreset>& SDKickMakerProcessor::presets()
{
    static const std::vector<KickPreset> p {
        //  Name                 Punch  Body  Länge Drive Rumble
        { "INDUSTRIAL HAMMER",   0.85f, 0.45f, 0.45f, 0.80f, 0.15f },
        { "WAREHOUSE RUMBLE",    0.60f, 0.60f, 0.55f, 0.65f, 0.75f },
        { "SCHRANZ PUNCH",       0.95f, 0.30f, 0.30f, 0.90f, 0.00f },
        { "DEEP TUNNEL",         0.50f, 0.80f, 0.80f, 0.45f, 0.50f },
        { "RAW DISTORTION",      0.75f, 0.55f, 0.50f, 1.00f, 0.30f },
        { "CLEAN SUB",           0.40f, 0.70f, 0.60f, 0.10f, 0.00f },
    };
    return p;
}

juce::AudioProcessorValueTreeState::ParameterLayout SDKickMakerProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> ps;

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; });

    ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Modus",
                                                          StringArray { "SYNTH", "SAMPLE" }, 0));
    ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Tonart", keyNames, 6));
    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tune", 1 }, "Tune",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
            { return (v > 0.04f ? "+" : "") + String (v, 1) + " st"; })));
    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "punch", 1 },  "Punch",  NormalisableRange<float> (0, 1), 0.85f, pct));
    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "body", 1 },   "Body",   NormalisableRange<float> (0, 1), 0.45f, pct));
    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "length", 1 }, "Laenge", NormalisableRange<float> (0, 1), 0.45f, pct));
    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 },  "Drive",  NormalisableRange<float> (0, 1), 0.80f, pct));
    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "rumble", 1 }, "Rumble", NormalisableRange<float> (0, 1), 0.15f, pct));
    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
        NormalisableRange<float> (-24.0f, 6.0f, 0.1f), -3.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
            { return String (v, 1) + " dB"; })));

    return { ps.begin(), ps.end() };
}

SDKickMakerProcessor::SDKickMakerProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SDKICK", createLayout())
{
    formats.registerBasicFormats();
}

bool SDKickMakerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

KickParams SDKickMakerProcessor::currentParams() const
{
    KickParams p;
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    p.sampleMode = v ("mode") > 0.5f;
    p.key      = juce::jlimit (0, 11, (int) std::round (v ("key")));
    p.tune     = v ("tune");
    p.punch    = v ("punch");
    p.body     = v ("body");
    p.length   = v ("length");
    p.drive    = v ("drive");
    p.rumble   = v ("rumble");
    p.outputDb = v ("output");
    p.bpm      = hostBpm.load();
    return p;
}

void SDKickMakerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.setParams (currentParams());
    engineBlock = juce::jmax (32, samplesPerBlock);
    engine.prepare (sampleRate, engineBlock);
    setLatencySamples (engine.getLatencySamples());
    monoBuf.assign ((size_t) juce::jmax (32, samplesPerBlock), 0.0f);
}

void SDKickMakerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                hostBpm = *bpm;

    engine.setParams (currentParams());

    if (previewRequested.exchange (false))
        engine.trigger (1.0f);

    int pos = 0;
    auto renderTo = [&] (int end)
    {
        while (pos < end)
        {
            const int chunk = juce::jmin (end - pos, engineBlock);
            engine.process (monoBuf.data(), chunk);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.copyFrom (ch, pos, monoBuf.data(), chunk);
            pos += chunk;
        }
    };

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            renderTo (juce::jlimit (0, n, meta.samplePosition));
            engine.trigger (0.35f + 0.65f * msg.getFloatVelocity());
        }
    }
    renderTo (n);
    midi.clear();

    float pk = buffer.getMagnitude (0, 0, n);
    float old = peak.load();
    while (pk > old && ! peak.compare_exchange_weak (old, pk)) {}
}

// ---------------------------------------------------------------------------
bool SDKickMakerProcessor::loadSampleFile (const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (f));
    if (reader == nullptr)
        return false;

    const int len = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 4.0));
    if (len < 32)
        return false;

    juce::AudioBuffer<float> tmp ((int) reader->numChannels, len);
    reader->read (&tmp, 0, len, 0, true, true);

    auto smp = std::make_shared<KickSample>();
    smp->audio.setSize (1, len);
    smp->audio.clear();
    for (int ch = 0; ch < tmp.getNumChannels(); ++ch)
        smp->audio.addFrom (0, 0, tmp, ch, 0, len, 1.0f / (float) tmp.getNumChannels());

    const float mag = smp->audio.getMagnitude (0, 0, len);
    if (mag > 0.0f) smp->audio.applyGain (1.0f / mag);

    smp->sampleRate = reader->sampleRate;
    smp->detectedHz = KickSample::detectPitch (smp->audio, reader->sampleRate);
    smp->name = f.getFileNameWithoutExtension();

    engine.setSample (smp);
    samplePath = f.getFullPathName();

    if (auto* m = apvts.getParameter ("mode"))
        m->setValueNotifyingHost (1.0f);
    return true;
}

void SDKickMakerProcessor::clearSample()
{
    engine.setSample (nullptr);
    samplePath.clear();
}

juce::String SDKickMakerProcessor::getSampleInfo() const
{
    auto s = engine.getSample();
    if (s == nullptr)
        return "Eigene Kick hier reinziehen (WAV/AIFF) - wird automatisch gestimmt";

    juce::String info = s->name;
    if (s->detectedHz > 0.0)
    {
        const double midi = 69.0 + 12.0 * std::log2 (s->detectedHz / 440.0);
        const int note = ((int) std::round (midi) % 12 + 12) % 12;
        info << "   |   erkannt: " << juce::String (s->detectedHz, 1) << " Hz (" << keyNames[note]
             << ")  ->  " << keyNames[currentParams().key];
    }
    else
        info << "   |   Tonhoehe nicht erkannt - wird nicht umgestimmt";
    return info;
}

void SDKickMakerProcessor::loadPreset (int index)
{
    const auto& p = presets();
    if (index < 0 || index >= (int) p.size()) return;
    const auto& pr = p[(size_t) index];
    auto set = [this] (const char* id, float v)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };
    set ("punch", pr.punch); set ("body", pr.body); set ("length", pr.length);
    set ("drive", pr.drive); set ("rumble", pr.rumble);
}

void SDKickMakerProcessor::toggleAB()
{
    const int cur = abIsB ? 1 : 0;
    abSlot[cur] = apvts.copyState();
    abIsB = ! abIsB;
    const int nxt = abIsB ? 1 : 0;
    if (abSlot[nxt].isValid())
        apvts.replaceState (abSlot[nxt].createCopy());
}

bool SDKickMakerProcessor::exportWav (const juce::File& f)
{
    const double rate = 44100.0;
    auto p = currentParams();
    auto data = KickEngine::renderOneShot (p, engine.getSample(), rate, p.beatSeconds());

    f.getParentDirectory().createDirectory();
    f.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream (f.createOutputStream());
    if (stream == nullptr) return false;

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), rate, 1, 24, {}, 0));
    if (writer == nullptr) return false;
    stream.release();

    juce::AudioBuffer<float> b (1, (int) data.size());
    b.copyFrom (0, 0, data.data(), (int) data.size());
    return writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

// ---------------------------------------------------------------------------
void SDKickMakerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("samplePath", samplePath, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SDKickMakerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            const juce::String path = state.getProperty ("samplePath").toString();
            apvts.replaceState (state);
            const float mode = apvts.getRawParameterValue ("mode")->load();
            if (path.isNotEmpty() && juce::File (path).existsAsFile())
            {
                loadSampleFile (juce::File (path));
                if (auto* m = apvts.getParameter ("mode"))
                    m->setValueNotifyingHost (mode);
            }
        }
    }
}

juce::AudioProcessorEditor* SDKickMakerProcessor::createEditor()
{
    return new SDKickMakerEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SDKickMakerProcessor();
}
