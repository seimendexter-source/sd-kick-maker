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
    const juce::Colour group    { 0xff222222 };
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

// Große Wellenform: Datei-Drop-Ziel + zwei verschiebbare Grenzen (Click | Body | Tail)
class WaveDisplay : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    WaveDisplay (SDKickMakerProcessor& p, const SDFonts& f);
    void setData (const std::vector<float>& d, double lengthSec);
    void paint (juce::Graphics&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { hover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { hover = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> plotArea() const;
    float xForSplit (float v) const;
    float splitForX (float x) const;
    float splitValue (const char* id) const;
    int handleAt (juce::Point<float> pos) const;

    SDKickMakerProcessor& proc;
    const SDFonts& fonts;
    std::vector<float> data;
    double seconds = 0.0;
    bool hover = false;
    int dragging = -1;   // 0 = clickSplit, 1 = bodySplit
};

// Kompakte Sample-Leiste: Dateiname, X (entladen), Ø (Phase)
class SampleDrop : public juce::Component
{
public:
    SampleDrop (SDKickMakerProcessor& p, const SDFonts& f);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    juce::String info;
private:
    juce::Rectangle<int> nameArea() const;
    SDKickMakerProcessor& proc;
    const SDFonts& fonts;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::TextButton clearBtn { "X" };
    juce::TextButton phaseBtn { juce::String::fromUTF8 ("\xc3\x98") };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phaseAtt;
};

// Button mit kleinem Vektor-Symbol (Play-Dreieck / Würfel)
class IconButton : public juce::TextButton
{
public:
    enum class Icon { none, play, dice };
    IconButton (const juce::String& buttonText, Icon i, const SDFonts& f) : juce::TextButton (buttonText), icon (i), fonts (f) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    Icon icon;
    const SDFonts& fonts;
};

class SDKickMakerEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SDKickMakerEditor (SDKickMakerProcessor&);
    ~SDKickMakerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void refreshWave();
    juce::String paramsHash() const;
    juce::Rectangle<float> meterBounds() const;

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
    juce::Slider dryWetKnob;
    std::unique_ptr<SA> dryWetAtt;

    juce::ComboBox presetBox, keyBox, modeBox;
    std::unique_ptr<CA> keyAtt, modeAtt;

    IconButton playBtn { "PLAY", IconButton::Icon::play, fonts };
    juce::TextButton exportBtn { "EXPORT WAV" };
    IconButton abBtn { "A/B", IconButton::Icon::none, fonts };
    IconButton randomBtn { "RANDOM", IconButton::Icon::dice, fonts };
    WaveDisplay wave;
    SampleDrop drop;

    std::array<juce::Rectangle<int>, 3> groupBounds;

    juce::String lastHash;
    int waveCountdown = 0;
    float meterDb = -60.0f;
    bool clipLed = false;
    std::unique_ptr<juce::FileChooser> saveChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SDKickMakerEditor)
};
