#include "PluginEditor.h"

using namespace juce;

namespace
{
    constexpr float kPi     = MathConstants<float>::pi;
    constexpr float kTwoPi  = MathConstants<float>::twoPi;
    constexpr float kHalfPi = MathConstants<float>::halfPi;

    String uiFamily()
    {
       #if JUCE_WINDOWS
        return "Segoe UI";
       #elif JUCE_MAC
        return "Helvetica Neue";
       #else
        return "DejaVu Sans";
       #endif
    }

    Font uiFont (float h, bool bold = false, float kern = 0.f, bool light = false)
    {
        auto opts = FontOptions (h).withName (uiFamily())
                                   .withStyle (bold ? "Bold" : (light ? "Light" : "Regular"));
        return Font (opts).withExtraKerningFactor (kern);
    }

    Colour hueCol (float hue, float alpha) { return Colour::fromHSV (hue, 0.7f, 1.f, alpha); }
    Colour fade (Colour c) { return c.withAlpha (0.f); }
}

//==============================================================================
//  LookAndFeel
//==============================================================================
VortexLookAndFeel::VortexLookAndFeel()
{
    setColour (PopupMenu::backgroundColourId,            Colour (0xff0d0a1f));
    setColour (PopupMenu::textColourId,                  Theme::ink);
    setColour (PopupMenu::highlightedBackgroundColourId, Theme::violet.withAlpha (0.55f));
    setColour (PopupMenu::highlightedTextColourId,       Colours::white);
    setColour (ComboBox::textColourId,                   Theme::ink);
    setColour (ComboBox::arrowColourId,                  Theme::cyan);
}

void VortexLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                          float a0, float a1, Slider& s)
{
    const Rectangle<float> area ((float) x, (float) y, (float) w, (float) h);
    const Colour accent ((uint32) (int) s.getProperties()["accent"]);

    const float side = jmin (area.getWidth(), area.getHeight() - 32.f);
    const Point<float> c (area.getCentreX(), area.getY() + side * 0.5f);
    const float rad = side * 0.5f - 14.f;
    const bool active = s.isMouseOverOrDragging();
    const float glow = jlimit (0.f, 1.f, 0.35f + pulse * 1.4f + (active ? 0.3f : 0.f));
    const bool bipolar = s.getMinimum() < 0.0;
    const float angle = a0 + pos * (a1 - a0);
    const float zeroAngle = bipolar ? (a0 + a1) * 0.5f : a0;
    const float trackR = rad + 8.f;

    // ореол
    {
        ColourGradient hg (accent.withAlpha (0.24f * glow), c, fade (accent), c.translated (rad * 1.45f, 0.f), true);
        g.setGradientFill (hg);
        g.fillEllipse (c.x - rad * 1.45f, c.y - rad * 1.45f, rad * 2.9f, rad * 2.9f);
    }

    // дорожка
    {
        Path track;
        track.addCentredArc (c.x, c.y, trackR, trackR, 0.f, a0, a1, true);
        g.setColour (Colour (0xff2a2345));
        g.strokePath (track, PathStrokeType (3.f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // значение (светящаяся дуга)
    if (std::abs (angle - zeroAngle) > 0.002f)
    {
        Path val;
        val.addCentredArc (c.x, c.y, trackR, trackR, 0.f, jmin (angle, zeroAngle), jmax (angle, zeroAngle), true);
        const PathStrokeType::JointStyle js = PathStrokeType::curved;
        g.setColour (accent.withAlpha (0.10f + 0.12f * glow)); g.strokePath (val, PathStrokeType (13.f, js, PathStrokeType::rounded));
        g.setColour (accent.withAlpha (0.28f));                g.strokePath (val, PathStrokeType (7.f,  js, PathStrokeType::rounded));
        g.setColour (accent);                                  g.strokePath (val, PathStrokeType (3.2f, js, PathStrokeType::rounded));
    }

    // тело
    {
        ColourGradient bg (Colour (0xff261c4a), c.translated (-rad * 0.5f, -rad * 0.6f),
                           Colour (0xff07050f), c.translated (rad * 0.7f, rad * 0.8f), false);
        g.setGradientFill (bg);
        g.fillEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f);
        g.setColour (accent.withAlpha (0.35f + 0.3f * glow));
        g.drawEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f, 1.4f);
    }

    // стрелка + точка на дуге
    const Point<float> dir (std::sin (angle), -std::cos (angle));
    g.setColour (Colours::white.withAlpha (0.92f));
    g.drawLine (Line<float> (c + dir * (rad * 0.30f), c + dir * (rad * 0.82f)), 2.6f);

    const Point<float> tip = c + dir * trackR;
    {
        ColourGradient tg (accent.withAlpha (0.8f), tip, fade (accent), tip.translated (11.f, 0.f), true);
        g.setGradientFill (tg);
        g.fillEllipse (tip.x - 11.f, tip.y - 11.f, 22.f, 22.f);
        g.setColour (Colours::white);
        g.fillEllipse (tip.x - 2.6f, tip.y - 2.6f, 5.2f, 5.2f);
    }

    // подписи
    auto textArea = area.withTop (area.getBottom() - 32.f);
    g.setFont (uiFont (13.f, true, 0.05f));
    g.setColour (active ? Colours::white : Theme::ink.withAlpha (0.9f));
    g.drawText (s.getTextFromValue (s.getValue()), textArea.removeFromTop (17.f), Justification::centred, false);
    g.setFont (uiFont (10.f, false, 0.28f));
    g.setColour (accent.withAlpha (0.85f));
    g.drawText (s.getName(), textArea, Justification::centred, false);
}

void VortexLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hi, bool down)
{
    const auto bounds = b.getLocalBounds().toFloat();
    const Colour accent = b.getProperties().contains ("accent")
                              ? Colour ((uint32) (int) b.getProperties()["accent"]) : Theme::violet;
    const bool on = b.getToggleState();
    const auto r = bounds.reduced (3.f);
    const float cr = r.getHeight() * 0.5f;

    if (on)
    {
        g.setColour (accent.withAlpha (0.22f));
        g.fillRoundedRectangle (bounds.reduced (1.f), bounds.getHeight() * 0.5f);
        ColourGradient fg (accent, r.getTopLeft(), accent.interpolatedWith (Theme::magenta, 0.55f), r.getBottomRight(), false);
        g.setGradientFill (fg);
        g.fillRoundedRectangle (r, cr);
    }
    else
    {
        g.setColour (Colours::white.withAlpha (hi ? 0.14f : 0.07f));
        g.fillRoundedRectangle (r, cr);
        g.setColour (accent.withAlpha (hi ? 0.85f : 0.45f));
        g.drawRoundedRectangle (r, cr, 1.2f);
    }
    if (down) { g.setColour (Colours::white.withAlpha (0.12f)); g.fillRoundedRectangle (r, cr); }
}

void VortexLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setFont (uiFont (11.5f, true, 0.3f));
    g.setColour (b.getToggleState() ? Colour (0xff06050c) : Theme::ink);
    g.drawText (b.getButtonText(), b.getLocalBounds(), Justification::centred, false);
}

void VortexLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool down, int, int, int, int, ComboBox& box)
{
    const auto r = Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (1.f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (Theme::violet.withAlpha ((box.isMouseOver() || down) ? 0.95f : 0.5f));
    g.drawRoundedRectangle (r, 8.f, 1.2f);

    Path arrow;
    const float ax = (float) w - 15.f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
    g.setColour (Theme::cyan);
    g.fillPath (arrow);
}

Font VortexLookAndFeel::getComboBoxFont (ComboBox&) { return uiFont (12.f, false, 0.12f); }
Font VortexLookAndFeel::getPopupMenuFont()          { return uiFont (13.f, false, 0.1f); }

//==============================================================================
//  PortalComponent
//==============================================================================
PortalComponent::PortalComponent (VortexAudioProcessor& p) : proc (p)
{
    setOpaque (false);
    setInterceptsMouseClicks (false, false);

    Random rng (1337);
    for (int i = 0; i < 70; ++i)
        stars.push_back ({ rng.nextFloat() * kTwoPi, 0.12f + rng.nextFloat() * 1.06f,
                           0.5f + rng.nextFloat(), 1.f + rng.nextFloat() * 1.8f,
                           rng.nextFloat() * kTwoPi, 1.f + rng.nextFloat() * 2.f,
                           0.4f + rng.nextFloat() * 0.6f, rng.nextBool() });

    lastMs = Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

void PortalComponent::resized()
{
    trailImage = Image (Image::ARGB, jmax (1, getWidth()), jmax (1, getHeight()), true);
    centre = getLocalBounds().toFloat().getCentre();
    ringR = jmin (getWidth(), getHeight()) * 0.5f - 40.f;
}

void PortalComponent::spawn (const GrainEvent& e)
{
    if (particles.size() > 420) return;

    const int pc = ((int) std::round (e.semis) % 12 + 12) % 12;
    const float hue = 0.48f + 0.42f * (float) pc / 12.f; // cyan -> violet -> magenta

    Particle p;
    p.a0      = e.pos * kTwoPi - kHalfPi;
    p.age     = 0.f;
    p.life    = jlimit (0.4f, 2.2f, e.lengthMs * 0.001f * 1.6f + 0.45f);
    p.size    = jlimit (1.8f, 6.5f, 2.f + e.lengthMs * 0.012f);
    p.hue     = hue;
    p.dir     = e.pan >= 0.f ? 1.f : -1.f;
    p.swirl   = 1.8f + std::abs (e.pan) * 2.2f;
    p.reverse = e.reverse;
    particles.push_back (p);

    if (flashes.size() < 64) flashes.push_back ({ e.pos * kTwoPi, 1.f, hue });
}

void PortalComponent::timerCallback()
{
    const double now = Time::getMillisecondCounterHiRes();
    const float dt = jlimit (0.001f, 0.05f, (float) ((now - lastMs) * 0.001));
    lastMs = now;

    bpm  = proc.visBpm.load();
    bars = proc.getBars();
    const double beats = (double) bars * 4.0;

    // плавная фаза, подтягиваемая к реальной фазе хоста
    displayPhase += ((double) bpm / 60.0) / beats * (double) dt;
    double diff = (double) proc.visPhase.load() - displayPhase;
    diff -= std::round (diff);
    displayPhase += diff * 0.18;
    displayPhase -= std::floor (displayPhase);

    level += (jlimit (0.f, 1.f, proc.visLevel.load() * 1.4f) - level) * 0.25f;
    const double beatPos = displayPhase * beats;
    kick = std::pow (1.f - (float) (beatPos - std::floor (beatPos)), 3.f);

    GrainEvent ev; int safety = 0;
    while (proc.engine.popEvent (ev) && safety++ < 48) spawn (ev);

    // звёзды
    for (auto& s : stars)
    {
        s.ang   += dt * s.spd * (0.06f + bpm / 120.f * 0.10f) / (0.35f + s.rad);
        s.phase += dt * s.tw;
    }

    // вспышки на кольце
    for (size_t i = 0; i < flashes.size();)
    {
        flashes[i].life -= dt * 2.8f;
        if (flashes[i].life <= 0.f) { flashes[i] = flashes.back(); flashes.pop_back(); }
        else ++i;
    }

    // частицы -> в слой следов
    if (trailImage.isValid())
    {
        trailImage.multiplyAllAlphas (0.87f);
        Graphics tg (trailImage);
        const float coreR = 14.f + 7.f * kick + 16.f * level;

        for (size_t i = 0; i < particles.size();)
        {
            auto& p = particles[i];
            p.age += dt;
            const float t = p.age / p.life;
            if (t >= 1.f) { p = particles.back(); particles.pop_back(); continue; }

            const float tt = p.reverse ? 1.f - t : t;
            const float r   = coreR * 1.2f + (ringR * 0.98f - coreR * 1.2f) * std::pow (1.f - tt, 1.6f);
            const float ang = p.a0 + p.dir * p.swirl * tt * (0.4f + tt);
            const float px = centre.x + r * std::cos (ang);
            const float py = centre.y + r * std::sin (ang);
            const float a  = jlimit (0.f, 1.f, std::sin (t * kPi) * 1.3f);
            const float s  = p.size;

            tg.setColour (hueCol (p.hue, a * 0.22f));
            tg.fillEllipse (px - s * 2.4f, py - s * 2.4f, s * 4.8f, s * 4.8f);
            tg.setColour (hueCol (p.hue, a * 0.85f));
            tg.fillEllipse (px - s * 0.7f, py - s * 0.7f, s * 1.4f, s * 1.4f);
            tg.setColour (Colours::white.withAlpha (a));
            tg.fillEllipse (px - s * 0.28f, py - s * 0.28f, s * 0.56f, s * 0.56f);
            ++i;
        }
    }

    repaint();
}

void PortalComponent::paint (Graphics& g)
{
    const auto c = centre;
    const float R = ringR;
    const float coreR = 14.f + 7.f * kick + 16.f * level;
    const float phaseAng = (float) displayPhase * kTwoPi; // от 12 часов по часовой

    // --- облако вихря ---
    {
        ColourGradient cg (Theme::violet.withAlpha (0.17f + 0.10f * level), c, fade (Theme::violet), c.translated (R, 0.f), true);
        cg.addColour (0.55, Theme::magenta.withAlpha (0.05f));
        g.setGradientFill (cg);
        g.fillEllipse (c.x - R, c.y - R, R * 2.f, R * 2.f);
    }

    // --- звёзды, вращающиеся вокруг портала ---
    for (auto& s : stars)
    {
        const float r = s.rad * R * 1.2f;
        const float tw = 0.5f + 0.5f * std::sin (s.phase);
        g.setColour ((s.warm ? Theme::magenta : Theme::cyan).withAlpha (0.12f + 0.55f * tw * s.bright));
        g.fillEllipse (c.x + r * std::cos (s.ang) - s.size * 0.5f, c.y + r * std::sin (s.ang) - s.size * 0.5f, s.size, s.size);
    }

    // --- спиральные рукава ---
    {
        const int arms = 5, seg = 26;
        const float rot = -phaseAng * 2.f;
        for (int k = 0; k < arms; ++k)
        {
            Point<float> prev;
            for (int s = 0; s <= seg; ++s)
            {
                const float u  = (float) s / (float) seg;
                const float r  = coreR * 1.6f + u * (R * 0.94f - coreR * 1.6f);
                const float th = (float) k * kTwoPi / (float) arms + rot + (1.f - u) * 3.6f;
                const Point<float> p (c.x + r * std::cos (th), c.y + r * std::sin (th));
                if (s > 0)
                {
                    const float a = std::pow (std::sin (u * kPi), 1.3f) * (0.35f + 0.5f * level + 0.25f * kick);
                    const Colour col = Theme::cyan.interpolatedWith (Theme::magenta, jlimit (0.f, 1.f, u * 0.9f + (float) k * 0.04f));
                    g.setColour (col.withAlpha (a * 0.16f)); g.drawLine (Line<float> (prev, p), 7.f);
                    g.setColour (col.withAlpha (a * 0.90f)); g.drawLine (Line<float> (prev, p), 1.6f);
                }
                prev = p;
            }
        }
    }

    // --- гранулы (слой следов) ---
    if (trailImage.isValid()) g.drawImageAt (trailImage, 0, 0);

    // --- кольцо-такт ---
    {
        const Rectangle<float> rr (c.x - R, c.y - R, R * 2.f, R * 2.f);
        const float boost = 0.8f + 0.5f * kick + 0.3f * level;
        g.setColour (Theme::violet.withAlpha (0.05f * boost)); g.drawEllipse (rr, 18.f);
        g.setColour (Theme::violet.withAlpha (0.08f * boost)); g.drawEllipse (rr, 11.f);
        g.setColour (Theme::violet.withAlpha (0.16f * boost)); g.drawEllipse (rr, 5.f);
        ColourGradient rg (Theme::cyan, c.x - R, c.y - R, Theme::magenta, c.x + R, c.y + R, false);
        g.setGradientFill (rg);
        g.drawEllipse (rr, 2.2f);
        g.setColour (Theme::ink.withAlpha (0.10f));
        g.drawEllipse (rr.reduced (R * 0.06f), 1.f);
    }

    // --- деления: доли и подсветка у плейхеда ---
    {
        const int beats = bars * 4;
        const float beatPos = (float) displayPhase * (float) beats;
        for (int bt = 0; bt < beats; ++bt)
        {
            const float ang = (float) bt / (float) beats * kTwoPi - kHalfPi;
            float d = std::abs (beatPos - (float) bt);
            d = jmin (d, (float) beats - d);
            const float nearV = std::exp (-d * 2.5f);
            const bool barStart = (bt % 4 == 0);
            const float len = (barStart ? 15.f : 8.f) + 7.f * nearV;
            const float r0 = R + 9.f, r1 = r0 + len;
            Colour col = barStart ? Theme::ink.withAlpha (0.75f) : Theme::dim.withAlpha (0.55f);
            col = col.interpolatedWith (Theme::cyan, nearV);
            g.setColour (col);
            g.drawLine (c.x + r0 * std::cos (ang), c.y + r0 * std::sin (ang),
                        c.x + r1 * std::cos (ang), c.y + r1 * std::sin (ang), barStart ? 2.2f : 1.4f);

            g.setColour (Theme::dim.withAlpha (0.28f));
            for (int s = 1; s < 4; ++s)
            {
                const float a2 = ang + (float) s / 4.f * kTwoPi / (float) beats;
                g.fillEllipse (c.x + (R + 9.f) * std::cos (a2) - 0.9f, c.y + (R + 9.f) * std::sin (a2) - 0.9f, 1.8f, 1.8f);
            }
        }
    }

    // --- вспышки на кольце: откуда гранула берёт звук ---
    for (auto& f : flashes)
    {
        Path arc;
        arc.addCentredArc (c.x, c.y, R, R, 0.f, f.ang - 0.06f, f.ang + 0.06f, true);
        g.setColour (hueCol (f.hue, f.life * 0.28f)); g.strokePath (arc, PathStrokeType (13.f));
        g.setColour (hueCol (f.hue, f.life));         g.strokePath (arc, PathStrokeType (3.5f));
    }

    // --- плейхед со шлейфом ---
    {
        for (int i = 0; i < 22; ++i)
        {
            const float a1 = phaseAng - (float) i * 0.03f, a0 = a1 - 0.03f;
            Path wedge;
            wedge.addPieSegment (c.x - R, c.y - R, R * 2.f, R * 2.f, a0, a1, 0.f);
            g.setColour (Theme::cyan.withAlpha (0.10f * std::pow (1.f - (float) i / 22.f, 2.f)));
            g.fillPath (wedge);
        }
        const Point<float> tip (c.x + R * std::sin (phaseAng), c.y - R * std::cos (phaseAng));
        ColourGradient lg (fade (Theme::cyan), c, Theme::cyan.withAlpha (0.85f), tip, false);
        g.setGradientFill (lg);
        g.drawLine (Line<float> (c, tip), 1.6f);

        ColourGradient hg (Colours::white.withAlpha (0.95f), tip, fade (Theme::cyan), tip.translated (17.f, 0.f), true);
        hg.addColour (0.3, Theme::cyan.withAlpha (0.6f));
        g.setGradientFill (hg);
        g.fillEllipse (tip.x - 17.f, tip.y - 17.f, 34.f, 34.f);
    }

    // --- ядро ---
    {
        ColourGradient halo (Theme::magenta.withAlpha (0.55f), c, fade (Theme::magenta), c.translated (coreR * 4.f, 0.f), true);
        halo.addColour (0.35, Theme::violet.withAlpha (0.25f));
        g.setGradientFill (halo);
        g.fillEllipse (c.x - coreR * 4.f, c.y - coreR * 4.f, coreR * 8.f, coreR * 8.f);

        ColourGradient orb (Colours::white, c, fade (Theme::cyan), c.translated (coreR, 0.f), true);
        orb.addColour (0.45, Theme::cyan.withAlpha (0.9f));
        g.setGradientFill (orb);
        g.fillEllipse (c.x - coreR, c.y - coreR, coreR * 2.f, coreR * 2.f);

        const float rr = coreR * 1.9f;
        for (int k = 0; k < 12; ++k)
        {
            const float a = phaseAng * 3.f + (float) k * kTwoPi / 12.f;
            Path arc;
            arc.addCentredArc (c.x, c.y, rr, rr, 0.f, a, a + 0.3f, true);
            g.setColour (Theme::violet.withAlpha (0.7f));
            g.strokePath (arc, PathStrokeType (1.4f));
        }
    }

    // --- инфо ---
    g.setFont (uiFont (10.5f, false, 0.3f));
    g.setColour (Theme::dim.withAlpha (0.9f));
    g.drawText (String ((int) std::round (bpm)) + " BPM    |    " + String (bars) + (bars == 1 ? " BAR" : " BARS"),
                Rectangle<float> (c.x - 90.f, c.y + R * 0.62f, 180.f, 14.f), Justification::centred, false);
}

//==============================================================================
//  Editor
//==============================================================================
VortexAudioProcessorEditor::VortexAudioProcessorEditor (VortexAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    portal = std::make_unique<PortalComponent> (proc);
    addAndMakeVisible (*portal);

    setupKnob (rateK,  VortexParams::pRate,    "RATE",    Theme::cyan,    rateA);
    setupKnob (sizeK,  VortexParams::pSize,    "SIZE",    Theme::violet,  sizeA);
    setupKnob (scanK,  VortexParams::pScan,    "SCAN",    Colour (0xff4dffc3), scanA);
    setupKnob (harmK,  VortexParams::pHarmony, "HARMONY", Theme::magenta, harmA);
    setupKnob (chaosK, VortexParams::pChaos,   "CHAOS",   Theme::amber,   chaosA);
    setupKnob (mixK,   VortexParams::pMix,     "MIX",     Colour (0xffc9b8ff), mixA);

    rateK.textFromValueFunction = [] (double v)
    {
        return String (VortexParams::rateTable[jlimit (0, VortexParams::numRates - 1, (int) std::round (v))].name);
    };
    sizeK.textFromValueFunction = [] (double v) { return String ((int) std::round (v)) + "%"; };
    scanK.textFromValueFunction = [] (double v)
    {
        if (std::abs (v) < 0.02) return String ("CENTER");
        return String (v > 0 ? "+" : "") + String (v, 2);
    };
    auto pct = [] (double v) { return String ((int) std::round (v * 100.0)) + "%"; };
    harmK.textFromValueFunction  = pct;
    chaosK.textFromValueFunction = pct;
    mixK.textFromValueFunction   = pct;

    setupCombo (keyBox,   VortexParams::pKey,    keyA);
    setupCombo (scaleBox, VortexParams::pScale,  scaleA);
    setupCombo (bufBox,   VortexParams::pBuffer, bufA);

    freezeBtn.setClickingTogglesState (true);
    freezeBtn.getProperties().set ("accent", (int) Theme::cyan.getARGB());
    addAndMakeVisible (freezeBtn);
    freezeA = std::make_unique<ButtonAtt> (proc.apvts, VortexParams::pFreeze, freezeBtn);

    for (auto* b : { &prevBtn, &nextBtn, &presetBtn })
    {
        b->getProperties().set ("accent", (int) Theme::violet.getARGB());
        addAndMakeVisible (*b);
    }
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    presetBtn.onClick = [this]
    {
        PopupMenu menu;
        for (int i = 0; i < numPresets; ++i)
            menu.addItem (i + 1, vortexPresets[i].name, true, i == proc.getCurrentProgram());

        Component::SafePointer<VortexAudioProcessorEditor> safe (this);
        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (presetBtn),
                            [safe] (int r)
                            {
                                if (safe != nullptr && r > 0)
                                {
                                    safe->proc.setCurrentProgram (r - 1);
                                    safe->refreshPresetName();
                                }
                            });
    };
    refreshPresetName();

    setSize (900, 600);
    startTimerHz (30);
}

VortexAudioProcessorEditor::~VortexAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VortexAudioProcessorEditor::setupKnob (Slider& s, const String& id, const String& name,
                                            Colour accent, std::unique_ptr<SliderAtt>& att)
{
    s.setName (name);
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (kPi * 1.25f, kPi * 2.75f, true);
    s.setMouseDragSensitivity (200);
    s.getProperties().set ("accent", (int) accent.getARGB());
    addAndMakeVisible (s);
    att = std::make_unique<SliderAtt> (proc.apvts, id, s);
    if (auto* prm = proc.apvts.getParameter (id))
        s.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
}

void VortexAudioProcessorEditor::setupCombo (ComboBox& box, const char* id, std::unique_ptr<ComboAtt>& att)
{
    if (auto* cp = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id)))
        box.addItemList (cp->choices, 1);
    box.setJustificationType (Justification::centred);
    addAndMakeVisible (box);
    att = std::make_unique<ComboAtt> (proc.apvts, id, box);
}

void VortexAudioProcessorEditor::stepPreset (int delta)
{
    proc.setCurrentProgram ((proc.getCurrentProgram() + delta + numPresets) % numPresets);
    refreshPresetName();
}

void VortexAudioProcessorEditor::refreshPresetName()
{
    const auto name = proc.getProgramName (proc.getCurrentProgram()).toUpperCase();
    if (presetBtn.getButtonText() != name) presetBtn.setButtonText (name);
}

void VortexAudioProcessorEditor::timerCallback()
{
    laf.pulse = portal->getPulse();
    for (auto* s : { &rateK, &sizeK, &scanK, &harmK, &chaosK, &mixK }) s->repaint();
    refreshPresetName();
}

void VortexAudioProcessorEditor::resized()
{
    const int w = getWidth(), h = getHeight();

    portal->setBounds (220, 60, 460, 460);

    rateK.setBounds  (40,  55, 130, 150);
    sizeK.setBounds  (40, 215, 130, 150);
    scanK.setBounds  (40, 375, 130, 150);
    harmK.setBounds  (730, 55, 130, 150);
    chaosK.setBounds (730, 215, 130, 150);
    mixK.setBounds   (730, 375, 130, 150);

    prevBtn.setBounds   (304, 12, 28, 32);
    presetBtn.setBounds (340, 12, 220, 32);
    nextBtn.setBounds   (568, 12, 28, 32);
    freezeBtn.setBounds (w - 150, 12, 120, 32);

    keyBox.setBounds   (271, 546, 90, 26);
    scaleBox.setBounds (375, 546, 140, 26);
    bufBox.setBounds   (529, 546, 100, 26);

    // статичный фон
    bgImage = Image (Image::RGB, jmax (1, w), jmax (1, h), false);
    Graphics g (bgImage);
    g.fillAll (Theme::bg);

    const auto pc = portal->getBounds().getCentre().toFloat();
    {
        ColourGradient cg (Theme::violet.withAlpha (0.30f), pc, fade (Theme::violet), pc.translated (380.f, 0.f), true);
        g.setGradientFill (cg); g.fillRect (0, 0, w, h);
        ColourGradient mg (Theme::magenta.withAlpha (0.13f), Point<float> ((float) w * 0.88f, (float) h * 0.92f),
                           fade (Theme::magenta), Point<float> ((float) w * 0.88f - 320.f, (float) h * 0.92f), true);
        g.setGradientFill (mg); g.fillRect (0, 0, w, h);
        ColourGradient yg (Theme::cyan.withAlpha (0.10f), Point<float> ((float) w * 0.08f, (float) h * 0.1f),
                           fade (Theme::cyan), Point<float> ((float) w * 0.08f + 320.f, (float) h * 0.1f), true);
        g.setGradientFill (yg); g.fillRect (0, 0, w, h);
    }
    {
        Random rng (7);
        for (int i = 0; i < 150; ++i)
        {
            const float sz = 0.8f + rng.nextFloat() * 1.0f;
            g.setColour ((rng.nextInt (3) == 0 ? Theme::cyan : Theme::ink).withAlpha (0.08f + rng.nextFloat() * 0.35f));
            g.fillEllipse (rng.nextFloat() * (float) w, rng.nextFloat() * (float) h, sz, sz);
        }
    }
    {
        ColourGradient vg (Colours::black.withAlpha (0.f), pc, Colours::black.withAlpha (0.65f), Point<float> (0.f, 0.f), true);
        g.setGradientFill (vg); g.fillRect (0, 0, w, h);

        g.setColour (Colours::white.withAlpha (0.025f)); g.fillRect (0, 0, w, 56);
        ColourGradient hl (fade (Theme::violet), 0.f, 56.f, fade (Theme::violet), (float) w, 56.f, false);
        hl.addColour (0.5, Theme::violet.withAlpha (0.6f));
        g.setGradientFill (hl); g.fillRect (0.f, 55.f, (float) w, 1.2f);
    }
}

void VortexAudioProcessorEditor::paint (Graphics& g)
{
    if (bgImage.isValid()) g.drawImageAt (bgImage, 0, 0);

    // название
    g.setGradientFill (ColourGradient (Theme::cyan, 30.f, 0.f, Theme::magenta, 170.f, 0.f, false));
    g.setFont (uiFont (26.f, true, 0.38f));
    g.drawText ("VORTEX", 30, 4, 220, 34, Justification::centredLeft, false);
    g.setFont (uiFont (8.5f, false, 0.35f));
    g.setColour (Theme::dim.withAlpha (0.8f));
    g.drawText ("BAR-LOCKED GRANULAR PORTAL", 32, 34, 220, 14, Justification::centredLeft, false);

    // подписи к выпадающим спискам
    g.setFont (uiFont (9.f, false, 0.3f));
    g.setColour (Theme::dim);
    g.drawText ("KEY",    keyBox.getBounds().withY (530).withHeight (14),   Justification::centred, false);
    g.drawText ("SCALE",  scaleBox.getBounds().withY (530).withHeight (14), Justification::centred, false);
    g.drawText ("BUFFER", bufBox.getBounds().withY (530).withHeight (14),   Justification::centred, false);

    // автор
    AttributedString as;
    as.setJustification (Justification::centredRight);
    as.append ("create by ", uiFont (10.f, false, 0.22f, true), Theme::dim.withAlpha (0.75f));
    as.append ("flmd",       uiFont (11.f, true,  0.30f),       Theme::cyan.withAlpha (0.95f));
    as.draw (g, Rectangle<float> ((float) getWidth() - 204.f, (float) getHeight() - 28.f, 188.f, 16.f));
}
