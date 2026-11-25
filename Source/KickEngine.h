#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>
#include <array>

// ============================================================================
//  SD KICK MAKER – Klangerzeugung (unabhängig von der Plugin-Hülle,
//  damit dieselbe Engine auch für Wellenform-Anzeige und WAV-Export rendert)
// ============================================================================

struct KickParams
{
    bool  sampleMode = false;
    int   key        = 6;      // 0 = C ... 11 = B   (Standard F#)
    float tune       = 0.0f;   // Halbtöne, -12 ... +12
    float punch      = 0.7f;   // 0 ... 1
    float body       = 0.55f;  // 0 ... 1
    float length     = 0.5f;   // 0 ... 1  (Anteil eines Beats)
    float drive      = 0.6f;   // 0 ... 1
    float rumble     = 0.0f;   // 0 ... 1
    float outputDb   = 0.0f;   // dB
    double bpm       = 160.0;
    float dryWet     = 1.0f;   // 0 = nur Original, 1 = nur bearbeitet
    float clickSplit = 0.25f;  // Grenze Click/Body (Anteil eines Beats)
    float bodySplit  = 0.65f;  // Grenze Body/Tail (Anteil eines Beats)
    bool  phaseInvert = false; // Sample-Phase drehen

    // Grundton der Kick in Hz (Oktave 1, z. B. F#1 = 46,2 Hz)
    double fundamentalHz() const
    {
        const double note = 24.0 + key + tune;   // MIDI 24 = C1
        return 440.0 * std::pow (2.0, (note - 69.0) / 12.0);
    }

    double beatSeconds() const { return 60.0 / juce::jlimit (60.0, 300.0, bpm); }
    double lengthSeconds() const { return beatSeconds() * (0.2 + 0.85 * length); }
};

// Geladenes Kick-Sample (mono, Spitze auf 1 normalisiert)
struct KickSample
{
    juce::AudioBuffer<float> audio;
    double sampleRate  = 44100.0;
    double detectedHz  = 0.0;   // erkannter Grundton (0 = unbekannt)
    juce::String name;

    // Grundton im "Körper" der Kick schätzen (Autokorrelation 30 ... 160 Hz)
    static double detectPitch (const juce::AudioBuffer<float>& mono, double sr);
};

class KickVoice
{
public:
    void prepare (double sr) { sampleRate = sr; active = false; }
    void start (const KickParams& p, float velocity, std::shared_ptr<const KickSample> smp);
    void release();                                // schnelles Ausblenden (Retrigger)
    bool isActive() const { return active; }

    // ein Sample erzeugen; env bekommt die aktuelle Hüllkurve (für Rumble-Ducking),
    // dry das unbearbeitete Signal (Original-Sample bzw. roher Synth)
    float next (float& env, float& dry);

private:
    double sampleRate = 44100.0;
    bool   active = false;
    bool   releasing = false;
    float  releaseGain = 1.0f;
    float  releaseCoef = 0.99f;

    long long t = 0;
    double phase = 0.0;
    double f0 = 46.0, fStart = 1000.0, tauPitch = 0.02;
    double lenSec = 0.3, holdSec = 0.05, curve = 2.0;
    float  punch = 0.5f, vel = 1.0f;
    float  clickTau = 0.0011f;
    float  polarity = 1.0f;
    bool   useSample = false;

    // Klick
    float noiseHpState = 0.0f, noisePrev = 0.0f;
    juce::Random rng;

    // Sample-Wiedergabe
    std::shared_ptr<const KickSample> sample;
    double samplePos = 0.0, sampleStep = 1.0;
};

class KickEngine
{
public:
    KickEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void setParams (const KickParams& p) { params = p; }
    const KickParams& getParams() const { return params; }
    void setSample (std::shared_ptr<const KickSample> s) { std::atomic_store (&sample, s); }
    std::shared_ptr<const KickSample> getSample() const { return std::atomic_load (&sample); }

    void trigger (float velocity);
    void process (float* mono, int numSamples);   // numSamples <= maxBlockSize
    int  getLatencySamples() const;

    // Komplette Kick offline rendern (für Anzeige und WAV-Export)
    static std::vector<float> renderOneShot (const KickParams& p,
                                             std::shared_ptr<const KickSample> smp,
                                             double sampleRate, double seconds);

private:
    KickParams params;
    std::shared_ptr<const KickSample> sample;

    double sr = 44100.0;
    int maxBlock = 512;

    std::array<KickVoice, 2> voices;
    int currentVoice = 0;

    juce::AudioBuffer<float> work, envBuf, dryBuf;
    std::vector<float> dryDelay;
    int dryDelayPos = 0;
    long long samplesSinceTrigger = 1 << 30;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // Nachbearbeitung
    float dcX = 0.0f, dcY = 0.0f;
    float toneState = 0.0f;

    // Rumble: kleines dunkles Hall-Netz + Filter + Verzerrung
    static constexpr int numLines = 4;
    std::array<std::vector<float>, numLines> lines;
    std::array<int, numLines> lineLen {}, linePos {};
    std::array<float, numLines> lineLp {};
    juce::dsp::IIR::Filter<float> rumbleLp1, rumbleLp2, rumbleHp;
    float duckSmooth = 0.0f;

    // Ausgang
    juce::SmoothedValue<float> outGain;
    juce::SmoothedValue<float> wetMix;
    float lastPrePeak = 0.0f;

public:
    // Spitzenpegel vor dem Schutzbegrenzer im letzten process()-Aufruf (für die Clip-LED)
    float getLastPrePeak() const { return lastPrePeak; }
};
