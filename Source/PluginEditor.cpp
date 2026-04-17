#include "PluginEditor.h"

TapePluginEditor::TapePluginEditor(TapePluginProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setSize(560, 380);
    startTimerHz(60);
}

void TapePluginEditor::timerCallback()
{
    float speed = *proc.apvts.getRawParameterValue("speed");
    reelPhase += (1.0f + speed) * 0.25f;

    // Auto-repeat for +/- step buttons while held
    if (pressedBtn == 1 || pressedBtn == 3)
    {
        ++holdFrames;
        // After 400ms initial delay (24 frames @ 60Hz), step every ~30ms (2 frames)
        if (holdFrames > 24 && (holdFrames % 2) == 0)
        {
            applyStep(pressedBtn == 1 ? -1 : +1);
        }
    }

    repaint();
}

void TapePluginEditor::resized()
{
    const int bw = 70, bh = 62;
    const int gap = 14;
    int totalW = bw * 5 + gap * 4;
    int x = (getWidth() - totalW) / 2;
    int y = getHeight() - bh - 34;

    maxSlowBtn  = { x,                      y, bw, bh };
    stepDownBtn = { x + (bw + gap) * 1,     y, bw, bh };
    stopBtn     = { x + (bw + gap) * 2,     y, bw, bh };
    stepUpBtn   = { x + (bw + gap) * 3,     y, bw, bh };
    maxFastBtn  = { x + (bw + gap) * 4,     y, bw, bh };
}

void TapePluginEditor::applyStep(int delta)
{
    auto* param = proc.apvts.getParameter("speed");
    float cur = *proc.apvts.getRawParameterValue("speed");
    const float stepSize = 1.0f / 24.0f;
    int curStep = (int)std::round(cur / stepSize);
    int newStep = juce::jlimit(-24, 24, curStep + delta);
    float newVal = newStep * stepSize;
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->convertTo0to1(newVal));
    param->endChangeGesture();
}

void TapePluginEditor::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    holdFrames = 0;
    if (maxSlowBtn.contains(pos))       { pressedBtn = 0; applyStep(-48); }
    else if (stepDownBtn.contains(pos)) { pressedBtn = 1; applyStep(-1); }
    else if (stopBtn.contains(pos))     {
        pressedBtn = 2;
        auto* param = proc.apvts.getParameter("speed");
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(0.0f));
        param->endChangeGesture();
        proc.resyncTape();
    }
    else if (stepUpBtn.contains(pos))   { pressedBtn = 3; applyStep(+1); }
    else if (maxFastBtn.contains(pos))  { pressedBtn = 4; applyStep(+48); }
    repaint();
}

void TapePluginEditor::mouseUp(const juce::MouseEvent&)
{
    pressedBtn = -1;
    holdFrames = 0;
    repaint();
}

// ------- Drawing helpers: minimal B&W ----------------------------------------

static const juce::Colour kInk      { 0xff111111 };
static const juce::Colour kPaper    { 0xfffafafa };
static const juce::Colour kSubtle   { 0xffe5e5e5 };
static const juce::Colour kMid      { 0xff888888 };

static void drawReel(juce::Graphics& g, juce::Point<float> c, float r, float phase)
{
    // Outer ring
    g.setColour(kInk);
    g.drawEllipse(c.x - r, c.y - r, r*2, r*2, 2.0f);

    // Filled disc (slightly inset)
    g.setColour(kInk);
    g.fillEllipse(c.x - r + 3, c.y - r + 3, (r - 3) * 2, (r - 3) * 2);

    // Inner hole
    g.setColour(kPaper);
    g.fillEllipse(c.x - r * 0.32f, c.y - r * 0.32f, r * 0.64f, r * 0.64f);
    g.setColour(kInk);
    g.drawEllipse(c.x - r * 0.32f, c.y - r * 0.32f, r * 0.64f, r * 0.64f, 1.5f);

    // Spokes (white cutouts on black disc)
    g.setColour(kPaper);
    for (int i = 0; i < 6; ++i) {
        float a = phase + i * juce::MathConstants<float>::twoPi / 6.0f;
        juce::Path spoke;
        float w = 0.08f; // spoke half-width in radians
        spoke.addPieSegment(c.x - r + 3, c.y - r + 3, (r - 3) * 2, (r - 3) * 2,
                            a - w, a + w, 0.45f);
        g.fillPath(spoke);
    }

    // Center dot
    g.setColour(kInk);
    g.fillEllipse(c.x - 2, c.y - 2, 4, 4);
}

static void drawBtn(juce::Graphics& g, juce::Rectangle<int> r,
                    const juce::String& label, bool pressed, bool filled)
{
    auto rf = r.toFloat();
    if (pressed) rf.translate(0, 1);

    if (filled) {
        g.setColour(kInk);
        g.fillRoundedRectangle(rf, 6.0f);
        g.setColour(kPaper);
    } else {
        g.setColour(kPaper);
        g.fillRoundedRectangle(rf, 6.0f);
        g.setColour(kInk);
        g.drawRoundedRectangle(rf, 6.0f, 1.5f);
    }

    if (pressed) {
        g.setColour(filled ? kPaper.withAlpha(0.15f) : kInk.withAlpha(0.08f));
        g.fillRoundedRectangle(rf, 6.0f);
        g.setColour(filled ? kPaper : kInk);
    }

    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText(label, rf.toNearestInt(), juce::Justification::centred);
}

void TapePluginEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Paper background
    g.fillAll(kPaper);

    // Thin outer frame
    g.setColour(kInk);
    g.drawRoundedRectangle(bounds.reduced(6), 14.0f, 1.5f);

    // Brand text
    g.setColour(kInk);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 22.0f, juce::Font::bold));
    g.drawText("GSMaiMai", bounds.reduced(30).removeFromTop(38).toNearestInt(),
               juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::plain));
    g.setColour(kMid);
    g.drawText("TAPE VARISPEED", bounds.reduced(30).removeFromTop(38).toNearestInt(),
               juce::Justification::centredRight);

    // Cassette window: thin-bordered rectangle
    auto window = juce::Rectangle<float>(40, 80, (float)getWidth() - 80, 170);
    g.setColour(kPaper);
    g.fillRoundedRectangle(window, 10.0f);
    g.setColour(kInk);
    g.drawRoundedRectangle(window, 10.0f, 1.5f);

    // Horizontal tape line across window
    g.setColour(kInk);
    float tapeY = window.getBottom() - 32.0f;
    g.fillRect(window.getX() + 20, tapeY - 1, window.getWidth() - 40, 2.0f);

    // Reels
    float reelR = 30.0f;
    float reelY = window.getCentreY() - 8.0f;
    drawReel(g, { window.getX() + 55.0f, reelY }, reelR, reelPhase);
    drawReel(g, { window.getRight() - 55.0f, reelY }, reelR, reelPhase);

    // Central numeric display (inverted panel: black on paper)
    auto lcd = juce::Rectangle<float>(window.getCentreX() - 72, window.getCentreY() - 40, 144, 78);
    g.setColour(kInk);
    g.fillRoundedRectangle(lcd, 6.0f);

    float speed = *proc.apvts.getRawParameterValue("speed");
    int step = proc.isEffectExhausted() ? 0 : (int)std::round(speed * 24.0f);
    juce::String disp = (step > 0 ? "+" : "") + juce::String(step);

    g.setColour(kPaper);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 38.0f, juce::Font::bold));
    g.drawText(disp, lcd.reduced(6).toNearestInt(), juce::Justification::centred);

    g.setColour(kPaper.withAlpha(0.55f));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.0f, juce::Font::plain));
    g.drawText("SPEED", lcd.removeFromBottom(14).toNearestInt().translated(0, -3),
               juce::Justification::centred);

    // Range scale under the window
    g.setColour(kMid);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.0f, juce::Font::plain));
    auto scale = juce::Rectangle<float>(40, 258, (float)getWidth() - 80, 14);
    g.drawText("-24", scale.toNearestInt(), juce::Justification::centredLeft);
    g.drawText("0",   scale.toNearestInt(), juce::Justification::centred);
    g.drawText("+24", scale.toNearestInt(), juce::Justification::centredRight);

    // Buttons (center STOP uses filled variant)
    drawBtn(g, maxSlowBtn,  juce::String::fromUTF8("\xe2\x97\x80\xe2\x97\x80"), pressedBtn == 0, false);
    drawBtn(g, stepDownBtn, juce::String::fromUTF8("\xe2\x80\x93"),              pressedBtn == 1, false);
    drawBtn(g, stopBtn,     juce::String::fromUTF8("\xe2\x96\xa0"),              pressedBtn == 2, true);
    drawBtn(g, stepUpBtn,   juce::String::fromUTF8("+"),                         pressedBtn == 3, false);
    drawBtn(g, maxFastBtn,  juce::String::fromUTF8("\xe2\x96\xb6\xe2\x96\xb6"), pressedBtn == 4, false);

    // Button captions
    g.setColour(kMid);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.0f, juce::Font::plain));
    g.drawText("MIN",  maxSlowBtn.translated(0, maxSlowBtn.getHeight() + 4),  juce::Justification::centred);
    g.drawText("-1",   stepDownBtn.translated(0, stepDownBtn.getHeight() + 4),juce::Justification::centred);
    g.drawText("STOP", stopBtn.translated(0, stopBtn.getHeight() + 4),        juce::Justification::centred);
    g.drawText("+1",   stepUpBtn.translated(0, stepUpBtn.getHeight() + 4),    juce::Justification::centred);
    g.drawText("MAX",  maxFastBtn.translated(0, maxFastBtn.getHeight() + 4),  juce::Justification::centred);
}
