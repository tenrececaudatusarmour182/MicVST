#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "audio/Metering.h"

namespace meterScale
{
    constexpr float minDb = -60.0f;   // linker Rand der Skala
    constexpr float maxDb =   0.0f;   // rechter Rand

    // dB -> normierte X-Position [0..1] über die Meterbreite.
    inline float dbToNorm (float db)
    {
        return juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
    }
}

// Horizontaler RMS-Balken (grün, von links) + Peak-Hold-Linie (gelb), mit
// dezenten dB-Gitterlinien. setLevel() wird vom GUI-Timer aufgerufen.
class LevelMeterComponent : public juce::Component
{
public:
    void setLevel (LevelReading r) { level = r; repaint(); }
    void paint (juce::Graphics& g) override;
private:
    LevelReading level;
};

// dB-Beschriftung unter den Metern, exakt an meterScale ausgerichtet.
class DbScaleComponent : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
};
