#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/AudioEngine.h"

// Kleines "i"-Icon, das bei Hover einen Hilfetext zeigt (via TooltipWindow im MainComponent).
struct InfoIcon : juce::Component, juce::SettableTooltipClient
{
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float d = juce::jmin (b.getWidth(), b.getHeight()) - 1.5f;   // Durchmesser = kleinere Seite
        auto circle = juce::Rectangle<float> (d, d).withCentre (b.getCentre());
        g.setColour (juce::Colours::grey);
        g.drawEllipse (circle, 1.2f);
        g.setColour (juce::Colours::lightgrey);
        g.setFont (d * 0.66f);
        g.drawText ("?", getLocalBounds(), juce::Justification::centred);
    }
};

// Kompakte Geräteauswahl: Input zuerst, dann Output (Output kennt "(none)").
// Darunter ein "Advanced"-Toggle, der Samplerate + Buffergröße ein-/ausklappt.
// Bewusst KEIN Input-Pegelmeter (oben gibt es den großen) und keine Kanal-Listen
// (MicVST verwaltet die Kanäle selbst auf Stereo).
class DevicePanel : public juce::Component, private juce::ChangeListener
{
public:
    explicit DevicePanel (AudioEngine&);
    ~DevicePanel() override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void refresh();        // Combos aus dem aktuellen Setup neu füllen
    void apply();          // aktuelle Auswahl -> AudioEngine
    void toggleAdvanced();

    AudioEngine& engine;
    juce::Label    inLabel  { {}, "Input" },  outLabel { {}, "Output" };
    InfoIcon       inInfo, outInfo;
    juce::ComboBox inBox, outBox;
    juce::TextButton advancedBtn { "Advanced" };
    juce::Label    srLabel  { {}, "Sample rate" }, bufLabel { {}, "Buffer size" };
    juce::ComboBox srBox, bufBox;

    juce::Array<double> sampleRates;   // ComboBox-Index -> Wert
    juce::Array<int>    bufferSizes;

    bool advancedVisible = false;
    bool updating = false;             // Re-entrancy-Guard beim Befüllen
};
