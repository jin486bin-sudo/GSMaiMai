#include "PluginEditor.h"

TapePluginEditor::TapePluginEditor(TapePluginProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setSize(560, 380);
    startTimerHz(30);
}

void TapePluginEditor::timerCallback()
{
    float speed = *proc.apvts.getRawParameterValue("speed");
    reelPhase += (1.0f + speed) * 0.25f;
    repaint();
}

void TapePluginEditor::resized()
{
    const int bw = 70, bh = 64;
    const int gap = 12;
    int totalW = bw * 5 + gap * 4;
    int x = (getWidth() - totalW) / 2;
    int y = getHeight() - bh - 34;

    maxSlowBtn  = { x,                              y, bw, bh };
    stepDownBtn = { x + (bw + gap) * 1,             y, bw, bh };
    stopBtn     = { x + (bw + gap) * 2,             y, bw, bh };
    stepUpBtn   = { x + (bw + gap) * 3,             y, bw, bh };
    maxFastBtn  = { x + (bw + gap) * 4,             y, bw, bh };
}

void TapePluginEditor::applyStep(int delta)
{
    auto* param = proc.apvts.getParameter("speed");
    float cur = *proc.apvts.getRawParameterValue("speed");
    const float stepSize = 1.0f / 20.0f;
    int curStep = (int)std::round(cur / stepSize);
    int newStep = juce::jlimit(-20, 20, curStep + delta);
    float newVal = newStep * stepSize;
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->convertTo0to1(newVal));
    param->endChangeGesture();
}

void TapePluginEditor::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    if (maxSlowBtn.contains(pos))       { pressedBtn = 0; applyStep(-40); }
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
    else if (maxFastBtn.contains(pos))  { pressedBtn = 4; applyStep(+40); }
    repaint();
}

void TapePluginEditor::mouseUp(const juce::MouseEvent&)
{
    pressedBtn = -1;
    repaint();
}

static void drawReel(juce::Graphics& g, juce::Point<float> c, float radius, float phase)
{
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillEllipse(c.x - radius, c.y - radius, radius*2, radius*2);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawEllipse(c.x - radius, c.y - radius, radius*2, radius*2, 2.0f);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillEllipse(c.x - radius*0.35f, c.y - radius*0.35f, radius*0.7f, radius*0.7f);
    g.setColour(juce::Colour(0xff555555));
    for (int i = 0; i < 6; ++i) {
        float a = phase + i * juce::MathConstants<float>::twoPi / 6.0f;
        float x1 = c.x + std::cos(a) * radius * 0.38f;
        float y1 = c.y + std::sin(a) * radius * 0.38f;
        float x2 = c.x + std::cos(a) * radius * 0.9f;
        float y2 = c.y + std::sin(a) * radius * 0.9f;
        g.drawLine(x1, y1, x2, y2, 3.0f);
    }
    g.setColour(juce::Colour(0xff888888));
    g.fillEllipse(c.x - 3, c.y - 3, 6, 6);
}

static void drawBtn(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& label,
                    juce::Colour top, juce::Colour bot, bool pressed)
{
    auto rf = r.toFloat();
    if (pressed) rf.translate(0, 2);

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(rf.translated(0, 3), 8.0f);

    juce::ColourGradient grad(top, rf.getX(), rf.getY(), bot, rf.getX(), rf.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(rf, 8.0f);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(rf, 8.0f, 1.5f);

    g.setColour(juce::Colours::white.withAlpha(pressed ? 0.05f : 0.25f));
    g.fillRoundedRectangle(rf.reduced(5).removeFromTop(rf.getHeight() * 0.35f), 4.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(17.0f, juce::Font::bold));
    g.drawText(label, rf.toNearestInt(), juce::Justification::centred);
}

void TapePluginEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Shell
    juce::ColourGradient bodyGrad(juce::Colour(0xffe8d9a8), 0, 0,
                                  juce::Colour(0xffbfa670), 0, (float)getHeight(), false);
    g.setGradientFill(bodyGrad);
    g.fillRoundedRectangle(bounds.reduced(8), 22.0f);
    g.setColour(juce::Colour(0xff5a4a20));
    g.drawRoundedRectangle(bounds.reduced(8), 22.0f, 2.5f);

    // Brand plate
    auto top = bounds.reduced(30).removeFromTop(34);
    g.setColour(juce::Colour(0xff3a2a10));
    g.fillRoundedRectangle(top, 6.0f);
    g.setColour(juce::Colour(0xffe8c060));
    g.setFont(juce::Font("Courier New", 22.0f, juce::Font::bold));
    g.drawText("GSMaiMai", top.toNearestInt(), juce::Justification::centred);

    // Cassette window
    auto window = juce::Rectangle<float>(40, 85, (float)getWidth() - 80, 170);
    g.setColour(juce::Colour(0xff1a1208));
    g.fillRoundedRectangle(window, 10.0f);
    g.setColour(juce::Colour(0xff000000));
    g.drawRoundedRectangle(window, 10.0f, 2.0f);

    auto label = window.reduced(14);
    juce::ColourGradient labelGrad(juce::Colour(0xffd4b896), label.getX(), label.getY(),
                                   juce::Colour(0xffa08858), label.getX(), label.getBottom(), false);
    g.setGradientFill(labelGrad);
    g.fillRoundedRectangle(label, 6.0f);

    // Reels — further apart for central display
    float reelR = 26.0f;
    float reelY = label.getCentreY();
    drawReel(g, { label.getX() + 40.0f, reelY }, reelR, reelPhase);
    drawReel(g, { label.getRight() - 40.0f, reelY }, reelR, reelPhase);

    // Central LCD-style digital display
    auto lcd = juce::Rectangle<float>(label.getCentreX() - 70, label.getCentreY() - 36, 140, 72);
    g.setColour(juce::Colour(0xff0a1a0a));
    g.fillRoundedRectangle(lcd, 6.0f);
    g.setColour(juce::Colour(0xff3a5a3a));
    g.drawRoundedRectangle(lcd, 6.0f, 1.5f);

    // Current step value
    float speed = *proc.apvts.getRawParameterValue("speed");
    int step = (int)std::round(speed * 20.0f);
    juce::String disp = (step > 0 ? "+" : "") + juce::String(step);

    g.setColour(juce::Colour(0xff7effa0)); // green LCD glow
    g.setFont(juce::Font("Courier New", 38.0f, juce::Font::bold));
    g.drawText(disp, lcd.reduced(4).toNearestInt(), juce::Justification::centred);

    // Tiny label under number
    g.setColour(juce::Colour(0xff4a7a4a));
    g.setFont(juce::Font("Courier New", 9.0f, juce::Font::bold));
    g.drawText("SPEED", lcd.removeFromBottom(14).toNearestInt().translated(0, -2),
               juce::Justification::centred);

    // Tape label brand strip
    g.setColour(juce::Colour(0xff8b4513));
    auto strip = label.withHeight(14).translated(0, label.getHeight() - 14);
    g.fillRect(strip);
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font("Courier New", 9.0f, juce::Font::bold));
    g.drawText("GSMaiMai TAPE   -20 SLOWEST   0 NORMAL   +20 FASTEST",
               strip.toNearestInt(), juce::Justification::centred);

    // Speaker grille
    auto grille = juce::Rectangle<float>(40, 265, (float)getWidth() - 80, 16);
    g.setColour(juce::Colour(0xff4a3820));
    for (int x = (int)grille.getX() + 8; x < grille.getRight() - 8; x += 10) {
        for (int y = (int)grille.getY() + 4; y < grille.getBottom() - 4; y += 8) {
            g.fillEllipse((float)x, (float)y, 4, 4);
        }
    }

    // Buttons
    juce::Colour blue1(0xff5a7ac0), blue2(0xff2a4080);
    juce::Colour red1(0xffc05a5a),  red2(0xff802a2a);
    juce::Colour gray1(0xff888888), gray2(0xff444444);

    drawBtn(g, maxSlowBtn,  juce::String::fromUTF8("\xe2\x97\x80\xe2\x97\x80"), // ◀◀
            blue1, blue2, pressedBtn == 0);
    drawBtn(g, stepDownBtn, juce::String::fromUTF8("\xe2\x97\x80"),             // ◀
            blue1.withBrightness(0.75f), blue2.withBrightness(0.75f), pressedBtn == 1);
    drawBtn(g, stopBtn,     juce::String::fromUTF8("\xe2\x96\xa0"),             // ■
            gray1, gray2, pressedBtn == 2);
    drawBtn(g, stepUpBtn,   juce::String::fromUTF8("\xe2\x96\xb6"),             // ▶
            red1.withBrightness(0.75f), red2.withBrightness(0.75f), pressedBtn == 3);
    drawBtn(g, maxFastBtn,  juce::String::fromUTF8("\xe2\x96\xb6\xe2\x96\xb6"), // ▶▶
            red1, red2, pressedBtn == 4);

    // Button labels
    g.setColour(juce::Colour(0xff3a2a10));
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText("MAX SLOW", maxSlowBtn.translated(0, maxSlowBtn.getHeight() + 3),  juce::Justification::centred);
    g.drawText("-1",       stepDownBtn.translated(0, stepDownBtn.getHeight() + 3),juce::Justification::centred);
    g.drawText("STOP",     stopBtn.translated(0, stopBtn.getHeight() + 3),        juce::Justification::centred);
    g.drawText("+1",       stepUpBtn.translated(0, stepUpBtn.getHeight() + 3),    juce::Justification::centred);
    g.drawText("MAX FAST", maxFastBtn.translated(0, maxFastBtn.getHeight() + 3),  juce::Justification::centred);
}
