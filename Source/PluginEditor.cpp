#include "PluginEditor.h"

TapePluginEditor::TapePluginEditor(TapePluginProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setSize(480, 360);
    startTimerHz(60);
}

void TapePluginEditor::timerCallback()
{
    float speed = *proc.apvts.getRawParameterValue("speed");
    reelPhase += (1.0f + speed) * 0.25f;

    // Auto-repeat for step buttons while held
    if (pressedBtn == 1 || pressedBtn == 3)
    {
        ++holdFrames;
        if (holdFrames > 24 && (holdFrames % 2) == 0)
            applyStep(pressedBtn == 1 ? -1 : +1);
    }
    repaint();
}

void TapePluginEditor::resized()
{
    const int bw = 80, bh = 62, gap = 16;
    int totalW = bw * 4 + gap * 3;
    int x = (getWidth() - totalW) / 2;
    int y = getHeight() - bh - 34;

    minBtn      = { x,                    y, bw, bh };
    stepDownBtn = { x + (bw + gap) * 1,   y, bw, bh };
    stopBtn     = { x + (bw + gap) * 2,   y, bw, bh };
    stepUpBtn   = { x + (bw + gap) * 3,   y, bw, bh };
}

void TapePluginEditor::applyStep(int delta)
{
    auto* param = proc.apvts.getParameter("speed");
    float cur = *proc.apvts.getRawParameterValue("speed");
    const float stepSize = 1.0f / 24.0f;
    int curStep = (int)std::round(cur / stepSize);
    int newStep = juce::jlimit(-24, 0, curStep + delta); // SLOW only: -24..0
    float newVal = newStep * stepSize;
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->convertTo0to1(newVal));
    param->endChangeGesture();
}

void TapePluginEditor::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    holdFrames = 0;
    if (minBtn.contains(pos))           { pressedBtn = 0; applyStep(-48); }
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
    repaint();
}

void TapePluginEditor::mouseUp(const juce::MouseEvent&)
{
    pressedBtn = -1;
    holdFrames = 0;
    repaint();
}

// ------- Drawing helpers: minimal B&W ----------------------------------------

static const juce::Colour kInk    { 0xff111111 };
static const juce::Colour kPaper  { 0xfffafafa };
static const juce::Colour kMid    { 0xff888888 };

static void drawReel(juce::Graphics& g, juce::Point<float> c, float r, float phase)
{
    g.setColour(kInk);
    g.drawEllipse(c.x - r, c.y - r, r*2, r*2, 2.0f);
    g.fillEllipse(c.x - r + 3, c.y - r + 3, (r-3)*2, (r-3)*2);

    g.setColour(kPaper);
    g.fillEllipse(c.x - r*0.32f, c.y - r*0.32f, r*0.64f, r*0.64f);
    g.setColour(kInk);
    g.drawEllipse(c.x - r*0.32f, c.y - r*0.32f, r*0.64f, r*0.64f, 1.5f);

    g.setColour(kPaper);
    for (int i = 0; i < 6; ++i) {
        float a = phase + i * juce::MathConstants<float>::twoPi / 6.0f;
        juce::Path spoke;
        spoke.addPieSegment(c.x-r+3, c.y-r+3, (r-3)*2, (r-3)*2, a-0.08f, a+0.08f, 0.45f);
        g.fillPath(spoke);
    }
    g.setColour(kInk);
    g.fillEllipse(c.x-2, c.y-2, 4, 4);
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

    g.fillAll(kPaper);
    g.setColour(kInk);
    g.drawRoundedRectangle(bounds.reduced(6), 14.0f, 1.5f);

    // Brand
    g.setColour(kInk);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 22.0f, juce::Font::bold));
    g.drawText("GSMaiMai", bounds.reduced(30).removeFromTop(38).toNearestInt(),
               juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::plain));
    g.setColour(kMid);
    g.drawText("TAPE SLOW", bounds.reduced(30).removeFromTop(38).toNearestInt(),
               juce::Justification::centredRight);

    // Cassette window
    auto window = juce::Rectangle<float>(40, 75, (float)getWidth() - 80, 160);
    g.setColour(kPaper);
    g.fillRoundedRectangle(window, 10.0f);
    g.setColour(kInk);
    g.drawRoundedRectangle(window, 10.0f, 1.5f);

    // Tape line
    float tapeY = window.getBottom() - 28.0f;
    g.fillRect(window.getX() + 20, tapeY - 1, window.getWidth() - 40, 2.0f);

    // Reels
    float reelR = 28.0f;
    float reelY = window.getCentreY() - 6.0f;
    drawReel(g, { window.getX() + 50.0f, reelY }, reelR, reelPhase);
    drawReel(g, { window.getRight() - 50.0f, reelY }, reelR, reelPhase);

    // LCD display
    auto lcd = juce::Rectangle<float>(window.getCentreX() - 60, window.getCentreY() - 34, 120, 66);
    g.setColour(kInk);
    g.fillRoundedRectangle(lcd, 6.0f);

    float speed = *proc.apvts.getRawParameterValue("speed");
    int step = (int)std::round(speed * 24.0f);
    step = juce::jlimit(-24, 0, step);
    juce::String disp = juce::String(step);

    g.setColour(kPaper);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 36.0f, juce::Font::bold));
    g.drawText(disp, lcd.reduced(6).toNearestInt(), juce::Justification::centred);

    g.setColour(kPaper.withAlpha(0.5f));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.0f, juce::Font::plain));
    g.drawText("SLOW", lcd.removeFromBottom(14).toNearestInt().translated(0, -2),
               juce::Justification::centred);

    // Scale
    g.setColour(kMid);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.0f, juce::Font::plain));
    auto scale = juce::Rectangle<float>(40, 243, (float)getWidth() - 80, 14);
    g.drawText("-24", scale.toNearestInt(), juce::Justification::centredLeft);
    g.drawText("0",   scale.toNearestInt(), juce::Justification::centredRight);

    // 4 buttons
    drawBtn(g, minBtn,      juce::String::fromUTF8("\xe2\x97\x80\xe2\x97\x80"), pressedBtn == 0, false);
    drawBtn(g, stepDownBtn, juce::String::fromUTF8("\xe2\x97\x80"),              pressedBtn == 1, false);
    drawBtn(g, stopBtn,     juce::String::fromUTF8("\xe2\x96\xa0"),              pressedBtn == 2, true);
    drawBtn(g, stepUpBtn,   juce::String::fromUTF8("\xe2\x96\xb6"),              pressedBtn == 3, false);

    g.setColour(kMid);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.0f, juce::Font::plain));
    g.drawText("MIN",  minBtn.translated(0, minBtn.getHeight() + 4),           juce::Justification::centred);
    g.drawText("-1",   stepDownBtn.translated(0, stepDownBtn.getHeight() + 4), juce::Justification::centred);
    g.drawText("STOP", stopBtn.translated(0, stopBtn.getHeight() + 4),         juce::Justification::centred);
    g.drawText("+1",   stepUpBtn.translated(0, stepUpBtn.getHeight() + 4),     juce::Justification::centred);
}
