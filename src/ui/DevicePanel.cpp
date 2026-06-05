#include "ui/DevicePanel.h"

DevicePanel::DevicePanel (AudioEngine& e) : engine (e)
{
    for (auto* l : { &inLabel, &outLabel })
        l->setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (inLabel);  addAndMakeVisible (inBox);
    addAndMakeVisible (outLabel); addAndMakeVisible (outBox);

    inInfo.setTooltip ("Select your microphone here");
    outInfo.setTooltip ("Select \"CABLE Input\" here, "
                        "and choose \"CABLE Output\" as the microphone in Discord etc.");
    addAndMakeVisible (inInfo);
    addAndMakeVisible (outInfo);

    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    statusLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    addAndMakeVisible (statusLabel);

    inBox.onChange  = [this] { apply(); };
    outBox.onChange = [this] { apply(); };

    engine.getDeviceManager().addChangeListener (this);
    refresh();
    startTimer (500);   // Info-Zeile (v. a. Geräte-/Plugin-Latenz) live halten
}

DevicePanel::~DevicePanel()
{
    stopTimer();
    engine.getDeviceManager().removeChangeListener (this);
}

void DevicePanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();   // Gerät extern geändert (z. B. ab-/angesteckt) -> Combos spiegeln
}

void DevicePanel::timerCallback()
{
    updateStatus();   // nur die Info-Zeile; Combos nicht anfassen (User könnte gerade wählen)
}

void DevicePanel::refresh()
{
    updating = true;   // verhindert, dass das Befüllen apply() auslöst

    inBox.clear (juce::dontSendNotification);
    outBox.clear (juce::dontSendNotification);

    auto& dm = engine.getDeviceManager();
    const auto setup = dm.getAudioDeviceSetup();

    if (auto* type = dm.getCurrentDeviceTypeObject())
    {
        const auto ins = type->getDeviceNames (true);
        for (int i = 0; i < ins.size(); ++i) inBox.addItem (ins[i], i + 1);
        const int si = ins.indexOf (setup.inputDeviceName);
        if (si >= 0) inBox.setSelectedId (si + 1, juce::dontSendNotification);

        outBox.addItem ("(none)", 1);   // leerer Output-Name = kein Host-Output
        const auto outs = type->getDeviceNames (false);
        for (int i = 0; i < outs.size(); ++i) outBox.addItem (outs[i], i + 2);
        const int so = outs.indexOf (setup.outputDeviceName);
        outBox.setSelectedId (setup.outputDeviceName.isNotEmpty() && so >= 0 ? so + 2 : 1,
                              juce::dontSendNotification);
    }

    updateStatus();

    updating = false;
}

void DevicePanel::updateStatus()
{
    auto& dm = engine.getDeviceManager();

    juce::String st = engine.isRunning() ? juce::String ("Active")
                                         : juce::String ("Idle - device disconnected");
    if (auto* dev = dm.getCurrentAudioDevice())
    {
        const double sr  = dev->getCurrentSampleRate();
        const int    buf = dev->getCurrentBufferSizeSamples();
        st << "   |   " << juce::String (sr, 0) << " Hz";
        st << "   |   buffer " << juce::String (buf);

        // Ende-zu-Ende durch MicVST: Geräte-Latenz (In+Out) + ein Block + Plugin-Latenz
        // des Graphen (Lookahead-Plugins melden diese via getLatencySamples()).
        const int    pluginLatency = engine.getGraph().getLatencySamples();
        const double lat = (dev->getInputLatencyInSamples() + dev->getOutputLatencyInSamples()
                            + buf + pluginLatency) / sr * 1000.0;
        st << "   |   latency " << juce::String (lat, 1) << " ms";
    }
    statusLabel.setText (st, juce::dontSendNotification);
}

void DevicePanel::apply()
{
    if (updating) return;

    auto& dm = engine.getDeviceManager();
    auto* type = dm.getCurrentDeviceTypeObject();
    if (type == nullptr) return;

    const auto ins  = type->getDeviceNames (true);
    const auto outs = type->getDeviceNames (false);

    const int ii = inBox.getSelectedId() - 1;
    juce::String input = juce::isPositiveAndBelow (ii, ins.size()) ? ins[ii] : juce::String();

    juce::String output;   // "(none)" (id 1) -> leer
    const int oid = outBox.getSelectedId();
    if (oid >= 2 && juce::isPositiveAndBelow (oid - 2, outs.size())) output = outs[oid - 2];

    // Samplerate/Buffer bleiben unverändert (0/0): im Shared-Modus gibt Windows sie vor.
    engine.setDeviceConfig (input, output, 0.0, 0);
}

void DevicePanel::resized()
{
    constexpr int rowH = 26, labelW = 84, gap = 4;
    auto r = getLocalBounds();

    auto row = [&]
    {
        auto a = r.removeFromTop (rowH);
        r.removeFromTop (gap);
        return a;
    };

    // Label + "?"-Icon in der Label-Spalte, danach die ComboBox.
    auto labelWithInfo = [labelW] (juce::Rectangle<int> r, juce::Label& lbl, InfoIcon& info)
    {
        auto col = r.removeFromLeft (labelW);
        info.setBounds (col.removeFromRight (18).reduced (2));
        lbl.setBounds (col);
        return r;
    };

    auto inRow = row();
    inBox.setBounds (labelWithInfo (inRow, inLabel, inInfo));
    auto outRow = row();
    outBox.setBounds (labelWithInfo (outRow, outLabel, outInfo));

    statusLabel.setBounds (row());   // dauerhafte Info-Zeile unter den Geräten
}
