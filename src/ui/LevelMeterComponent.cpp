#include "ui/LevelMeterComponent.h"

namespace
{
    // Gitter-/Beschriftungsmarken (dB). 0 dB rechts.
    const int kTicks[] = { -60, -48, -36, -24, -12, -6, 0 };
}

void LevelMeterComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black);
    g.fillRect (b);

    auto dbX = [&] (float db) { return b.getX() + meterScale::dbToNorm (db) * b.getWidth(); };

    // Dezente vertikale Gitterlinien an den Skalenmarken.
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    for (int db : kTicks)
        g.drawVerticalLine ((int) dbX ((float) db), b.getY(), b.getBottom());

    // RMS-Balken von links.
    const float rmsDb = juce::Decibels::gainToDecibels (level.rms, meterScale::minDb);
    const float rmsX  = dbX (rmsDb);
    if (rmsX > b.getX())
    {
        g.setColour (juce::Colours::limegreen);
        g.fillRect (juce::Rectangle<float> (b.getX(), b.getY(), rmsX - b.getX(), b.getHeight()));
    }

    // Peak-Hold als vertikale gelbe Linie.
    const float peakDb = juce::Decibels::gainToDecibels (level.peak, meterScale::minDb);
    if (peakDb > meterScale::minDb)
    {
        g.setColour (juce::Colours::yellow);
        const float px = dbX (peakDb);
        g.drawLine (px, b.getY(), px, b.getBottom(), 2.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawRect (b, 1.0f);
}

void DbScaleComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colours::lightgrey);
    g.setFont (11.0f);

    for (int db : kTicks)
    {
        const float x = b.getX() + meterScale::dbToNorm ((float) db) * b.getWidth();
        // Mittiger Text um die Markierung; Ränder etwas einrücken, damit nichts abgeschnitten wird.
        auto just = db == kTicks[0] ? juce::Justification::centredLeft
                  : db == 0         ? juce::Justification::centredRight
                                    : juce::Justification::centred;
        juce::Rectangle<float> r (x - 18.0f, b.getY(), 36.0f, b.getHeight());
        g.drawText (juce::String (db), r, just);
    }
}
