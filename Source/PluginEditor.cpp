#include "PluginEditor.h"
#include "BinaryData.h"

// ============================================================================
SDFonts::SDFonts()
{
    anton = juce::Typeface::createSystemTypefaceFor (BinaryData::AntonRegular_ttf, BinaryData::AntonRegular_ttfSize);
    mont  = juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize);
}
juce::Font SDFonts::head (float h) const { return juce::Font (juce::FontOptions (anton).withHeight (h).withHorizontalScale (1.08f)); }
juce::Font SDFonts::body (float h) const { return juce::Font (juce::FontOptions (mont).withHeight (h)); }

// ============================================================================
SDLookAndFeel::SDLookAndFeel (const SDFonts& f) : fonts (f)
{
    setColour (juce::ComboBox::backgroundColourId, sdcol::panel);
    setColour (juce::ComboBox::outlineColourId, sdcol::line);
    setColour (juce::ComboBox::textColourId, sdcol::text);
    setColour (juce::ComboBox::arrowColourId, sdcol::dim);
    setColour (juce::PopupMenu::backgroundColourId, sdcol::panel);
    setColour (juce::PopupMenu::textColourId, sdcol::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, sdcol::red);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, sdcol::panel);
    setColour (juce::TextButton::textColourOffId, sdcol::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, sdcol::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void SDLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float start, float end, juce::Slider&)
{
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float r = size * 0.5f;

    // Bahn
    juce::Path track;
    track.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, start, end, true);
    g.setColour (sdcol::line);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Wert (rot)
    const float ang = start + pos * (end - start);
    juce::Path val;
    val.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, start, ang, true);
    g.setColour (sdcol::red);
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knopf
    const float kr = r - 8.0f;
    g.setColour (sdcol::knob);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (sdcol::line);
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

    // Zeiger
    const juce::Point<float> tip (c.x + (kr - 4.0f) * std::sin (ang), c.y - (kr - 4.0f) * std::cos (ang));
    g.setColour (sdcol::text);
    g.drawLine ({ c, tip }, 2.5f);
}

void SDLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? sdcol::red : (over ? sdcol::panel.brighter (0.08f) : sdcol::panel));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (b.getToggleState() ? sdcol::red : sdcol::line);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

juce::Font SDLookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return fonts.head ((float) h * 0.5f); }

void SDLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (sdcol::panel);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.hasKeyboardFocus (true) ? sdcol::red : sdcol::line);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (sdcol::dim);
    g.fillPath (arrow);
}

juce::Font SDLookAndFeel::getComboBoxFont (juce::ComboBox&) { return fonts.body (13.0f); }
juce::Font SDLookAndFeel::getPopupMenuFont() { return fonts.body (14.0f); }

juce::Label* SDLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (fonts.body (12.0f));
    l->setColour (juce::Label::textColourId, sdcol::text);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

// ============================================================================
void WaveDisplay::setData (const std::vector<float>& d, double lengthSec)
{
    data = d; seconds = lengthSec; repaint();
}

void WaveDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (sdcol::panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (sdcol::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

    const float mid = r.getCentreY();
    g.drawHorizontalLine ((int) mid, r.getX() + 6.0f, r.getRight() - 6.0f);

    if (! data.empty())
    {
        const int W = (int) r.getWidth() - 12;
        const float half = r.getHeight() * 0.42f;
        juce::Path top, bottom;
        juce::RectangleList<float> bars;
        g.setColour (sdcol::red);
        for (int x = 0; x < W; ++x)
        {
            const size_t a = (size_t) ((double) x / W * (double) data.size());
            const size_t b = juce::jmax (a + 1, (size_t) ((double) (x + 1) / W * (double) data.size()));
            float mn = 0.0f, mx = 0.0f;
            for (size_t i = a; i < b && i < data.size(); ++i) { mn = juce::jmin (mn, data[i]); mx = juce::jmax (mx, data[i]); }
            const float y1 = mid - mx * half, y2 = mid - mn * half;
            g.fillRect (r.getX() + 6.0f + (float) x, y1, 1.0f, juce::jmax (1.0f, y2 - y1));
        }
    }

    g.setColour (sdcol::dim);
    g.setFont (fonts.body (11.0f));
    g.drawText ("CLICK", r.reduced (10.0f, 6.0f), juce::Justification::topLeft);
    g.drawText ("BODY", r.reduced (10.0f, 6.0f), juce::Justification::centredTop);
    g.drawText ("TAIL / RUMBLE", r.reduced (10.0f, 6.0f), juce::Justification::topRight);
    g.drawText (juce::String (seconds * 1000.0, 0) + " ms  =  1 Beat", r.reduced (10.0f, 6.0f), juce::Justification::bottomRight);
}

// ============================================================================
void SampleDrop::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (hover ? sdcol::panel.brighter (0.08f) : sdcol::panel);
    g.fillRoundedRectangle (r, 6.0f);
    juce::Path border; border.addRoundedRectangle (r, 6.0f);
    juce::Path dashed;
    const float dash[] = { 5.0f, 4.0f };
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, border, dash, 2);
    g.setColour (hover ? sdcol::red : sdcol::line);
    g.fillPath (dashed);

    g.setColour (sdcol::text);
    g.setFont (fonts.head (14.0f));
    g.drawText ("SAMPLE", r.reduced (12.0f, 10.0f), juce::Justification::topLeft);
    g.setColour (sdcol::dim);
    g.setFont (fonts.body (12.0f));
    g.drawFittedText (info, r.reduced (12.0f, 10.0f).withTrimmedTop (22.0f).toNearestInt(),
                      juce::Justification::topLeft, 3);
}

void SampleDrop::filesDropped (const juce::StringArray& files, int, int)
{
    hover = false;
    for (auto& f : files)
        if (proc.loadSampleFile (juce::File (f)))
            break;
    repaint();
}

void SampleDrop::mouseUp (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addItem (1, "Sample entfernen");
        m.showMenuAsync ({}, [this] (int r) { if (r == 1) proc.clearSample(); });
        return;
    }
    chooser = std::make_unique<juce::FileChooser> ("Kick-Sample waehlen", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f.existsAsFile()) proc.loadSampleFile (f);
                          });
}

// ============================================================================
SDKickMakerEditor::SDKickMakerEditor (SDKickMakerProcessor& p)
    : AudioProcessorEditor (&p), proc (p), drop (p, fonts)
{
    setLookAndFeel (&lnf);
    logo = juce::ImageCache::getFromMemory (BinaryData::sd_logo_png, BinaryData::sd_logo_pngSize);

    const char* ids[6]    = { "punch", "body", "length", "drive", "rumble", "tune" };
    const char* labels[6] = { "PUNCH", "BODY", "LAENGE", "DRIVE", "RUMBLE", "TUNE" };
    for (int i = 0; i < 6; ++i)
    {
        auto& k = knobs[(size_t) i];
        k.label = labels[i];
        k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 90, 18);
        k.slider.setDoubleClickReturnValue (false, 0.0);
        addAndMakeVisible (k.slider);
        k.att = std::make_unique<SA> (proc.apvts, ids[i], k.slider);
    }
    knobs[2].label = juce::String::fromUTF8 ("L\xc3\x84NGE");

    outKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    outKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    outKnob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    outKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 80, 16);
    addAndMakeVisible (outKnob);
    outAtt = std::make_unique<SA> (proc.apvts, "output", outKnob);

    int id = 1;
    for (auto& pr : SDKickMakerProcessor::presets())
        presetBox.addItem (pr.name, id++);
    presetBox.setTextWhenNothingSelected ("PRESET");
    presetBox.onChange = [this] { proc.loadPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    keyBox.addItemList (SDKickMakerProcessor::keyNames, 1);
    addAndMakeVisible (keyBox);
    keyAtt = std::make_unique<CA> (proc.apvts, "key", keyBox);

    modeBox.addItemList ({ "SYNTH", "SAMPLE" }, 1);
    addAndMakeVisible (modeBox);
    modeAtt = std::make_unique<CA> (proc.apvts, "mode", modeBox);

    playBtn.onClick = [this] { proc.preview(); };
    abBtn.onClick = [this]
    {
        proc.toggleAB();
        abBtn.setButtonText (proc.isOnB() ? "B" : "A");
    };
    abBtn.setTooltip ("Zwischen zwei Einstellungen A und B wechseln");
    exportBtn.onClick = [this]
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("SD KICK MAKER");
        dir.createDirectory();
        const auto prm = proc.currentParams();
        auto name = "SD_KICK_" + SDKickMakerProcessor::keyNames[prm.key].replace ("#", "is")
                    + "_" + juce::String (juce::roundToInt (prm.bpm)) + "BPM_"
                    + juce::Time::getCurrentTime().formatted ("%H%M%S") + ".wav";
        saveChooser = std::make_unique<juce::FileChooser> ("Kick als WAV speichern", dir.getChildFile (name), "*.wav");
        saveChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      auto f = fc.getResult();
                                      if (f != juce::File()) proc.exportWav (f.withFileExtension ("wav"));
                                  });
    };
    for (auto* b : { &playBtn, &exportBtn, &abBtn })
        addAndMakeVisible (*b);

    addAndMakeVisible (wave);
    addAndMakeVisible (drop);

    setSize (800, 500);
    refreshWave();
    startTimerHz (25);
}

SDKickMakerEditor::~SDKickMakerEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

juce::String SDKickMakerEditor::paramsHash() const
{
    const auto p = proc.currentParams();
    auto s = proc.apvts.copyState().toXmlString();
    s << (int) p.bpm << (proc.getSampleInfo());
    return juce::String (s.hashCode64());
}

void SDKickMakerEditor::refreshWave()
{
    const auto p = proc.currentParams();
    auto data = KickEngine::renderOneShot (p, proc.getSamplePtr(), 22050.0, p.beatSeconds());
    wave.setData (data, p.beatSeconds());
}

void SDKickMakerEditor::timerCallback()
{
    const auto h = paramsHash();
    if (h != lastHash)
    {
        lastHash = h;
        waveCountdown = 3;   // kurz warten, während am Regler gedreht wird
        drop.info = proc.getSampleInfo();
        drop.repaint();
    }
    if (waveCountdown > 0 && --waveCountdown == 0)
        refreshWave();

    const float pk = proc.getAndResetPeak();
    const float db = juce::Decibels::gainToDecibels (pk, -60.0f);
    meterDb = db > meterDb ? db : meterDb - 1.5f;
    repaint (getLocalBounds().removeFromBottom (130).removeFromRight (250));
    repaint (getLocalBounds().removeFromTop (64));
}

void SDKickMakerEditor::paint (juce::Graphics& g)
{
    g.fillAll (sdcol::bg);

    // Kopf
    if (logo.isValid())
    {
        const float h = 34.0f, w = h * (float) logo.getWidth() / (float) logo.getHeight();
        g.drawImage (logo, juce::Rectangle<float> (20.0f, 15.0f, w, h), juce::RectanglePlacement::centred);
    }
    g.setFont (fonts.head (26.0f));
    g.setColour (sdcol::text);
    const juce::String t1 ("SD KICK ");
    const int tw = juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t1));
    g.drawText (t1, 120, 14, tw + 4, 36, juce::Justification::centredLeft);
    g.setColour (sdcol::red);
    g.drawText ("MAKER", 120 + tw, 14, 120, 36, juce::Justification::centredLeft);

    g.setFont (fonts.body (11.0f));
    g.setColour (sdcol::dim);
    g.drawText ("PRESET", presetBox.getX(), 4, 100, 12, juce::Justification::left);
    g.drawText ("MODUS", modeBox.getX(), 4, 100, 12, juce::Justification::left);
    g.drawText ("TONART", keyBox.getX(), 4, 100, 12, juce::Justification::left);
    g.drawText ("BPM", 724, 4, 60, 12, juce::Justification::left);
    g.setFont (fonts.body (14.0f));
    g.setColour (sdcol::text);
    g.drawText (juce::String (juce::roundToInt (proc.getHostBpm())), 724, 18, 60, 30, juce::Justification::centredLeft);

    // Reglernamen
    g.setFont (fonts.head (14.0f));
    g.setColour (sdcol::dim);
    for (auto& k : knobs)
        g.drawText (k.label, k.slider.getX(), k.slider.getY() - 18, k.slider.getWidth(), 16, juce::Justification::centred);
    g.drawText ("OUTPUT", outKnob.getX(), outKnob.getY() - 18, outKnob.getWidth(), 16, juce::Justification::centred);

    // Pegelanzeige
    auto m = juce::Rectangle<float> ((float) outKnob.getRight() + 6.0f, (float) outKnob.getY(), 10.0f, (float) outKnob.getHeight() - 4.0f);
    g.setColour (sdcol::panel);
    g.fillRoundedRectangle (m, 3.0f);
    const float lvl = juce::jlimit (0.0f, 1.0f, (meterDb + 48.0f) / 48.0f);
    auto fill = m.withTrimmedTop (m.getHeight() * (1.0f - lvl));
    g.setColour (meterDb > -1.0f ? sdcol::red : sdcol::text.withAlpha (0.85f));
    g.fillRoundedRectangle (fill, 3.0f);

    g.setFont (fonts.body (10.0f));
    g.setColour (sdcol::dim);
    g.drawText ("SEIMEN DEXTER  |  DEXIT RECORDS", 20, getHeight() - 20, 300, 14, juce::Justification::left);
}

void SDKickMakerEditor::resized()
{
    presetBox.setBounds (380, 18, 180, 30);
    modeBox.setBounds (570, 18, 80, 30);
    keyBox.setBounds (660, 18, 56, 30);

    wave.setBounds (20, 70, 760, 140);

    const int kw = 760 / 6;
    for (int i = 0; i < 6; ++i)
        knobs[(size_t) i].slider.setBounds (20 + i * kw + 10, 248, kw - 20, 104);

    drop.setBounds (20, 378, 400, 90);
    outKnob.setBounds (436, 392, 76, 80);
    playBtn.setBounds (548, 380, 110, 40);
    abBtn.setBounds (668, 380, 112, 40);
    exportBtn.setBounds (548, 428, 232, 40);
}
