#include "KickEngine.h"

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;
}

// ============================================================================
//  Tonhöhen-Erkennung für Samples
// ============================================================================
double KickSample::detectPitch (const juce::AudioBuffer<float>& mono, double sr)
{
    if (mono.getNumSamples() < 64 || sr <= 0.0)
        return 0.0;

    // auf ca. 4 kHz heruntermitteln
    const int decim = juce::jmax (1, (int) std::round (sr / 4000.0));
    const double dsr = sr / decim;
    const float* src = mono.getReadPointer (0);

    // Ende der hörbaren Kick suchen (unter ca. -30 dB)
    const float pk = mono.getMagnitude (0, 0, mono.getNumSamples());
    int audibleEnd = mono.getNumSamples();
    for (int i = mono.getNumSamples() - 1; i > 0; --i)
        if (std::abs (src[i]) > 0.03f * pk) { audibleEnd = i; break; }

    // im ausgeklungenen Teil messen (Pitch-Sweep ist dort vorbei)
    const int start = juce::jmax ((int) (0.060 * sr), (int) (0.35 * audibleEnd));
    const int end   = juce::jmin (audibleEnd, start + (int) (0.30 * sr));
    if (end - start < (int) (0.04 * sr))
        return 0.0;

    std::vector<float> x;
    for (int i = start; i + decim <= end; i += decim)
    {
        float s = 0.0f;
        for (int k = 0; k < decim; ++k) s += src[i + k];
        x.push_back (s / (float) decim);
    }

    const int n = (int) x.size();
    const int minLag = (int) (dsr / 160.0);
    const int maxLag = juce::jmin (n / 2, (int) (dsr / 30.0));
    if (maxLag <= minLag + 2)
        return 0.0;

    double best = 0.0; int bestLag = 0;
    std::vector<double> corr ((size_t) maxLag + 2, 0.0);
    for (int lag = minLag; lag <= maxLag + 1 && lag < n; ++lag)
    {
        double num = 0.0, e1 = 0.0, e2 = 0.0;
        for (int i = 0; i + lag < n; ++i)
        {
            num += x[(size_t) i] * x[(size_t) (i + lag)];
            e1  += x[(size_t) i] * x[(size_t) i];
            e2  += x[(size_t) (i + lag)] * x[(size_t) (i + lag)];
        }
        const double c = (e1 > 0 && e2 > 0) ? num / std::sqrt (e1 * e2) : 0.0;
        corr[(size_t) lag] = c;
        if (lag <= maxLag && c > best) { best = c; bestLag = lag; }
    }

    if (bestLag == 0 || best < 0.3)
        return 0.0;

    // parabolische Feinjustierung
    double lag = bestLag;
    if (bestLag > minLag && bestLag < maxLag)
    {
        const double a = corr[(size_t) bestLag - 1], b = corr[(size_t) bestLag], c = corr[(size_t) bestLag + 1];
        const double d = a - 2.0 * b + c;
        if (std::abs (d) > 1e-9) lag += 0.5 * (a - c) / d;
    }
    return dsr / lag;
}

// ============================================================================
//  Stimme
// ============================================================================
void KickVoice::start (const KickParams& p, float velocity, std::shared_ptr<const KickSample> smp)
{
    active = true; releasing = false; releaseGain = 1.0f;
    releaseCoef = (float) std::exp (-1.0 / (0.004 * sampleRate));   // 4 ms
    t = 0; phase = 0.0;
    vel = velocity;
    punch = p.punch;

    f0 = p.fundamentalHz();
    fStart = f0 * std::pow (2.0, 1.0 + p.punch * 4.6);                 // bis ca. 2 kHz
    tauPitch = (0.002 + 0.018 * p.body) * (1.2 - 0.6 * p.punch);

    lenSec  = p.lengthSeconds();
    holdSec = lenSec * (0.04 + 0.5 * p.body);
    curve   = 1.1 + 2.6 * (1.0 - p.body);

    noiseHpState = noisePrev = 0.0f;

    useSample = p.sampleMode && smp != nullptr && smp->audio.getNumSamples() > 0;
    sample = useSample ? smp : nullptr;
    samplePos = 0.0;
    sampleStep = 1.0;

    if (useSample)
    {
        double ratio = 1.0;
        if (smp->detectedHz > 0.0)
        {
            // Zielton in der Oktave suchen, die am nächsten am Original liegt
            double bestR = 1.0, bestDist = 1e9;
            for (int k = -3; k <= 3; ++k)
            {
                const double r = (f0 * std::pow (2.0, k)) / smp->detectedHz;
                const double dist = std::abs (std::log2 (r));
                if (dist < bestDist) { bestDist = dist; bestR = r; }
            }
            ratio = bestR;
        }
        sampleStep = ratio * smp->sampleRate / sampleRate;
    }
}

void KickVoice::release()
{
    if (active) releasing = true;
}

float KickVoice::next (float& env)
{
    env = 0.0f;
    if (! active)
        return 0.0f;

    const double ts = (double) t / sampleRate;

    // Lautstärke-Hüllkurve
    double a;
    if (ts < 0.0004)          a = ts / 0.0004;
    else if (ts < holdSec)    a = 1.0;
    else
    {
        const double x = (ts - holdSec) / juce::jmax (1.0e-4, lenSec - holdSec);
        if (x >= 1.0) { active = false; return 0.0f; }
        a = std::pow (1.0 - x, curve);
    }

    if (releasing)
    {
        releaseGain *= releaseCoef;
        if (releaseGain < 1.0e-4f) { active = false; return 0.0f; }
        a *= releaseGain;
    }

    float s = 0.0f;

    if (! useSample)
    {
        const double f = f0 + (fStart - f0) * std::exp (-ts / tauPitch);
        phase += twoPi * f / sampleRate;
        if (phase > twoPi) phase -= twoPi;
        s = (float) std::sin (phase);

        // Klick: hochpassgefiltertes Rauschen, sehr kurz
        const float nz = rng.nextFloat() * 2.0f - 1.0f;
        noiseHpState = 0.6f * (noiseHpState + nz - noisePrev);
        noisePrev = nz;
        const float clickEnv = (float) std::exp (-ts / 0.0011);
        s += punch * 0.55f * clickEnv * noiseHpState;
    }
    else
    {
        const auto& buf = sample->audio;
        const int len = buf.getNumSamples();
        const int i0 = (int) samplePos;
        if (i0 + 1 >= len) { active = false; return 0.0f; }
        const float frac = (float) (samplePos - i0);
        const float* d = buf.getReadPointer (0);
        s = d[i0] + frac * (d[i0 + 1] - d[i0]);
        samplePos += sampleStep;

        // Punch = Transient-Verstärkung
        s *= 1.0f + punch * 2.2f * (float) std::exp (-ts / 0.006);
    }

    ++t;
    env = (float) a;
    return s * (float) a * vel;
}

// ============================================================================
//  Engine
// ============================================================================
KickEngine::KickEngine()
{
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
}

void KickEngine::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = juce::jmax (1, maxBlockSize);

    for (auto& v : voices) v.prepare (sr);

    work.setSize (1, maxBlock, false, true, true);
    envBuf.setSize (1, maxBlock, false, true, true);
    oversampler->initProcessing ((size_t) maxBlock);

    // Rumble-Netz: Längen leicht teilerfremd
    const double ms[numLines] = { 31.3, 37.9, 43.7, 53.1 };
    for (int i = 0; i < numLines; ++i)
    {
        lineLen[(size_t) i] = juce::jmax (2, (int) (ms[i] * 0.001 * sr));
        lines[(size_t) i].assign ((size_t) lineLen[(size_t) i], 0.0f);
    }

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 1 };
    rumbleLp1.prepare (spec); rumbleLp2.prepare (spec); rumbleHp.prepare (spec);
    *rumbleLp1.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 150.0f, 0.7f);
    *rumbleLp2.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 150.0f, 0.9f);
    *rumbleHp.coefficients  = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 28.0f, 0.7f);

    outGain.reset (sr, 0.02);
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (params.outputDb));

    reset();
}

void KickEngine::reset()
{
    oversampler->reset();
    dcX = dcY = toneState = 0.0f;
    for (int i = 0; i < numLines; ++i)
    {
        std::fill (lines[(size_t) i].begin(), lines[(size_t) i].end(), 0.0f);
        linePos[(size_t) i] = 0;
        lineLp[(size_t) i] = 0.0f;
    }
    rumbleLp1.reset(); rumbleLp2.reset(); rumbleHp.reset();
    duckSmooth = 0.0f;
}

int KickEngine::getLatencySamples() const
{
    return (int) std::round (oversampler->getLatencyInSamples());
}

void KickEngine::trigger (float velocity)
{
    voices[(size_t) currentVoice].release();
    currentVoice = 1 - currentVoice;
    voices[(size_t) currentVoice].start (params, velocity, getSample());
}

void KickEngine::process (float* out, int n)
{
    jassert (n <= maxBlock);
    float* w = work.getWritePointer (0);
    float* e = envBuf.getWritePointer (0);

    // 1) Stimmen erzeugen
    for (int i = 0; i < n; ++i)
    {
        float e0 = 0.0f, e1 = 0.0f;
        w[i] = voices[0].next (e0) + voices[1].next (e1);
        e[i] = juce::jmax (e0, e1);
    }

    // 2) Verzerrung in 4-facher Überabtastung
    {
        float* chans[1] = { w };
        juce::dsp::AudioBlock<float> block (chans, 1, (size_t) n);
        auto up = oversampler->processSamplesUp (block);
        float* u = up.getChannelPointer (0);
        const size_t un = up.getNumSamples();

        const float d = params.drive;
        const float g = 1.0f + 42.0f * d * d;
        const float bias = 0.18f * d;
        const float tb = std::tanh (bias);
        const float foldMix = juce::jlimit (0.0f, 1.0f, (d - 0.55f) / 0.45f);
        const float norm = 1.0f / std::tanh (juce::jmax (1.0f, g * 0.9f));

        for (size_t i = 0; i < un; ++i)
        {
            float y = (std::tanh (g * u[i] + bias) - tb) * norm;
            if (foldMix > 0.0f)
            {
                const float f = std::sin (y * juce::MathConstants<float>::pi * (0.5f + 0.75f * foldMix));
                y = (1.0f - foldMix) * y + foldMix * f;
            }
            u[i] = y;
        }
        oversampler->processSamplesDown (block);
    }

    // 3) DC-Sperre + sanfte Höhenabsenkung (ab ca. 14 kHz)
    const float dcR = 1.0f - (float) (twoPi * 15.0 / sr);
    const float toneA = (float) std::exp (-twoPi * 14000.0 / sr);
    for (int i = 0; i < n; ++i)
    {
        const float x = w[i];
        dcY = x - dcX + dcR * dcY;
        dcX = x;
        toneState = dcY + toneA * (toneState - dcY);
        w[i] = toneState;
    }

    // 4) Rumble
    const float rumble = params.rumble;
    if (rumble > 0.001f)
    {
        const float fb = 0.80f + 0.175f * rumble;
        const float lpA = (float) std::exp (-twoPi * 320.0 / sr);
        const float rDrive = 1.5f + 6.0f * rumble;
        const float rNorm = 1.0f / std::tanh (rDrive * 0.5f);
        const float duckA = (float) std::exp (-1.0 / (0.012 * sr));

        for (int i = 0; i < n; ++i)
        {
            float o[numLines];
            for (int k = 0; k < numLines; ++k)
                o[k] = lines[(size_t) k][(size_t) linePos[(size_t) k]];

            // Hadamard-Mischung
            const float h0 = 0.5f * ( o[0] + o[1] + o[2] + o[3]);
            const float h1 = 0.5f * ( o[0] - o[1] + o[2] - o[3]);
            const float h2 = 0.5f * ( o[0] + o[1] - o[2] - o[3]);
            const float h3 = 0.5f * ( o[0] - o[1] - o[2] + o[3]);
            const float hm[numLines] = { h0, h1, h2, h3 };

            const float in = w[i] * 0.5f;
            for (int k = 0; k < numLines; ++k)
            {
                auto& lp = lineLp[(size_t) k];
                lp = hm[k] * fb + lpA * (lp - hm[k] * fb);
                lines[(size_t) k][(size_t) linePos[(size_t) k]] = in + lp;
                if (++linePos[(size_t) k] >= lineLen[(size_t) k]) linePos[(size_t) k] = 0;
            }

            float r = 0.5f * (o[0] + o[1] + o[2] + o[3]);
            r = rumbleHp.processSample (rumbleLp2.processSample (rumbleLp1.processSample (r)));
            r = std::tanh (r * rDrive) * rNorm;

            // Ducking: Rumble weicht der Kick
            const float target = juce::jlimit (0.0f, 1.0f, e[i] * 1.6f);
            duckSmooth = target > duckSmooth ? target : target + duckA * (duckSmooth - target);
            const float duck = (1.0f - duckSmooth) * (1.0f - duckSmooth);

            w[i] += r * duck * rumble * 0.55f;
        }
    }

    // 5) Ausgang + Schutzbegrenzer
    outGain.setTargetValue (juce::Decibels::decibelsToGain (params.outputDb));
    for (int i = 0; i < n; ++i)
    {
        float y = w[i] * outGain.getNextValue();
        const float ay = std::abs (y);
        if (ay > 0.9f)
            y = std::copysign (0.9f + 0.09f * std::tanh ((ay - 0.9f) / 0.09f), y);
        out[i] = y;
    }
}

std::vector<float> KickEngine::renderOneShot (const KickParams& p,
                                              std::shared_ptr<const KickSample> smp,
                                              double sampleRate, double seconds)
{
    KickEngine eng;
    eng.setParams (p);
    eng.setSample (smp);
    const int block = 512;
    eng.prepare (sampleRate, block);

    const int lat = eng.getLatencySamples();
    const int total = (int) (seconds * sampleRate) + lat;
    std::vector<float> outv ((size_t) total, 0.0f);

    eng.trigger (1.0f);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = juce::jmin (block, total - pos);
        eng.process (outv.data() + pos, n);
    }
    outv.erase (outv.begin(), outv.begin() + lat);
    return outv;
}
