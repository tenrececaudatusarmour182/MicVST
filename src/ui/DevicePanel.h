#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/AudioEngine.h"

// Kleines "?"-Icon, das bei Hover einen Hilfetext zeigt (via TooltipWindow im MainComponent).
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
// Darunter eine dauerhaft sichtbare Info-Zeile (Active / Samplerate / Buffer / Latenz).
// Samplerate und Buffergröße sind bewusst REINE Readouts: im WASAPI-Shared-Modus gibt
// Windows genau eine Mixer-Periode vor, es gibt also nichts einzustellen.
// Bewusst KEIN Input-Pegelmeter (oben gibt es den großen) und keine Kanal-Listen
// (MicVST verwaltet die Kanäle selbst auf Stereo).
class DevicePanel : public juce::Component,
                    private juce::ChangeListener,
                    private juce::Timer
{
public:
    explicit DevicePanel (AudioEngine&);
    ~DevicePanel() override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refresh();        // Input/Output-Combos aus dem aktuellen Setup neu füllen
    void updateStatus();   // nur die Info-Zeile (Active / Samplerate / Buffer / Latenz)
    void apply();          // aktuelle Auswahl -> AudioEngine

    AudioEngine& engine;
    juce::Label    inLabel  { {}, "Input" },  outLabel { {}, "Output" };
    InfoIcon       inInfo, outInfo;
    juce::ComboBox inBox, outBox;
    juce::Label    statusLabel;   // Active / Samplerate / Buffer / Latenz (immer sichtbar)

    bool updating = false;        // Re-entrancy-Guard beim Befüllen
};
