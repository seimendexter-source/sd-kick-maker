#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace sdcol
{
    const juce::Colour bg       { 0xff0d0d0d };
    const juce::Colour panel    { 0xff171717 };
    const juce::Colour line     { 0xff2c2c2c };
    const juce::Colour knob     { 0xff1d1d1d };
    const juce::Colour red      { 0xffd6302c };
    const juce::Colour text     { 0xffe8e8e8 };
    const juce::Colour dim      { 0xff8a8a8a };
}

struct SDFonts
{
    juce::Typeface::Ptr anton, mont;
    SDFonts();
    juce::Font head (float h) const;
    juce::Font body (float h) const;
};

class SDLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit SDLookAndFeel (const SDFonts& f);
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
private:
    const SDFonts& fonts;
};

class WaveDisplay : public juce::Component
{
public:
    explicit WaveDisplay (const SDFonts& f) : fonts (f) {}
    void setData (const std::vector<float>& d, double lengthSec);
    void paint (juce::Graphics&) override;
private:
    const SDFonts& fonts;
    std::vector<float> data;
    double seconds = 0.0;
};

class SampleDrop : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    SampleDrop (SDKickMakerProcessor& p, const SDFonts& f) : proc (p), fonts (f) {}
    void paint (juce::Graphics&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void fileDragEnter (const juce::StringArray&, int, int) override { hover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { hover = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;
    void mouseUp (const juce::MouseEvent&) override;
    juce::String info;
private:
    SDKickMakerProcessor& proc;
    const SDFonts& fonts;
    bool hover = false;
    std::unique_ptr<juce::FileChooser> chooser;
};

class SDKickMakerEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SDKickMakerEditor (SDKickMakerProcessor&);
    ~SDKickMakerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshWave();
    juce::String paramsHash() const;

    SDKickMakerProcessor& proc;
    SDFonts fonts;
    SDLookAndFeel lnf { fonts };
    juce::Image logo;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob { juce::Slider slider; juce::String label; std::unique_ptr<SA> att; };
    std::array<Knob, 6> knobs;
    juce::Slider outKnob;
    std::unique_ptr<SA> outAtt;

    juce::ComboBox presetBox, keyBox, modeBox;
    std::unique_ptr<CA> keyAtt, modeAtt;

    juce::TextButton playBtn { "PLAY" }, exportBtn { "EXPORT WAV" }, abBtn { "A" };
    WaveDisplay wave { fonts };
    SampleDrop drop;

    juce::String lastHash;
    int waveCountdown = 0;
    float meterDb = -60.0f;
    std::unique_ptr<juce::FileChooser> saveChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SDKickMakerEditor)
};
