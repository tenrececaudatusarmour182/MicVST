#include "ui/MainComponent.h"
#include "audio/PluginChain.h"

MainComponent::MainComponent (AudioEngine& e) : engine (e)
{
    devicePanel = std::make_unique<DevicePanel> (engine);
    addAndMakeVisible (*devicePanel);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);
    addAndMakeVisible (dbScale);
    addAndMakeVisible (inLabel);
    addAndMakeVisible (outLabel);
    inLabel.setJustificationType (juce::Justification::centredLeft);
    outLabel.setJustificationType (juce::Justification::centredLeft);

    pluginList = std::make_unique<PluginListView> (engine);
    addAndMakeVisible (*pluginList);

    addAndMakeVisible (statusLabel);
    statusLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (autostartToggle);
    autostartToggle.setTooltip ("Launch MicVST silently into the tray when Windows starts");
    autostartToggle.setToggleState (AutostartRegistry::isEnabled(), juce::dontSendNotification);
    autostartToggle.onClick = [this] { AutostartRegistry::setEnabled (autostartToggle.getToggleState()); };

    cableHint.setColour (juce::HyperlinkButton::textColourId, juce::Colours::orange);
    cableHint.setJustificationType (juce::Justification::centredLeft);
    cableHint.setTooltip ("https://vb-audio.com/Cable/");
    addChildComponent (cableHint);   // Sichtbarkeit steuert updateCableHint()

    engine.onStatusChanged = [this] { updateStatus(); updateCableHint(); };
    updateStatus();
    updateCableHint();

    setSize (560, 560);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    engine.onStatusChanged = nullptr;
}

void MainComponent::timerCallback()
{
    inMeter.setLevel (engine.inputLevel());
    outMeter.setLevel (engine.outputLevel());
    updateStatus();
    // Mit der Registry synchron halten (z. B. wenn der Tray den Autostart umschaltet).
    autostartToggle.setToggleState (AutostartRegistry::isEnabled(), juce::dontSendNotification);
}

void MainComponent::updateStatus()
{
    juce::String t = engine.isRunning() ? juce::String ("Active")
                                        : juce::String ("Idle - device disconnected");
    if (auto* d = engine.getDeviceManager().getCurrentAudioDevice())
        t << "  |  " << juce::String (d->getCurrentSampleRate(), 0) << " Hz";
    if (auto* d = engine.getDeviceManager().getCurrentAudioDevice())
    {
        const double lat = (d->getInputLatencyInSamples() + d->getOutputLatencyInSamples()
                            + d->getCurrentBufferSizeSamples()) / d->getCurrentSampleRate() * 1000.0;
        t << "  |  latency ~" << juce::String (lat, 1) << " ms";
    }
    statusLabel.setText (t, juce::dontSendNotification);
}

void MainComponent::updateCableHint()
{
    // Hinweis nur zeigen, wenn KEIN virtuelles Kabel installiert ist. Wird bei Geräte-
    // Änderungen aufgerufen (z. B. nach VB-Cable-Installation verschwindet er von selbst).
    const bool noCable = engine.detectCableOutput().isEmpty();
    if (noCable != cableHint.isVisible())
    {
        cableHint.setVisible (noCable);
        resized();
    }
}

void MainComponent::resized()
{
    auto r = getLocalBounds().reduced (8);
    auto bottomRow = r.removeFromBottom (22);
    autostartToggle.setBounds (bottomRow.removeFromRight (130));
    statusLabel.setBounds (bottomRow);
    r.removeFromBottom (4);

    // Horizontale Meter: In-Zeile, Out-Zeile, gemeinsame dB-Skala darunter.
    constexpr int labelW = 40;
    auto inRow = r.removeFromTop (22);
    inLabel.setBounds (inRow.removeFromLeft (labelW));
    inMeter.setBounds (inRow.reduced (2, 1));
    r.removeFromTop (4);
    auto outRow = r.removeFromTop (22);
    outLabel.setBounds (outRow.removeFromLeft (labelW));
    outMeter.setBounds (outRow.reduced (2, 1));
    auto scaleRow = r.removeFromTop (16);
    scaleRow.removeFromLeft (labelW);                 // bündig unter den Metern
    dbScale.setBounds (scaleRow.reduced (2, 0));
    r.removeFromTop (8);

    if (cableHint.isVisible())
    {
        cableHint.setBounds (r.removeFromTop (22));
        r.removeFromTop (4);
    }

    auto bottom = r.removeFromBottom (juce::jmax (180, r.getHeight() / 2));
    pluginList->setBounds (bottom);
    devicePanel->setBounds (r);
}
