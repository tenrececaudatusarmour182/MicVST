#include "ui/DevicePanel.h"

DevicePanel::DevicePanel (AudioEngine& e) : engine (e)
{
    for (auto* l : { &inLabel, &outLabel, &srLabel, &bufLabel })
        l->setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (inLabel);  addAndMakeVisible (inBox);
    addAndMakeVisible (outLabel); addAndMakeVisible (outBox);
    addAndMakeVisible (advancedBtn);

    inInfo.setTooltip ("Select your microphone here");
    outInfo.setTooltip ("Select \"CABLE Input\" here, "
                        "and choose \"CABLE Output\" as the microphone in Discord etc.");
    addAndMakeVisible (inInfo);
    addAndMakeVisible (outInfo);

    // Advanced-Controls starten versteckt (Sichtbarkeit über den Toggle).
    addChildComponent (srLabel);  addChildComponent (srBox);
    addChildComponent (bufLabel); addChildComponent (bufBox);

    advancedBtn.setClickingTogglesState (true);
    advancedBtn.onClick = [this] { toggleAdvanced(); };

    inBox.onChange  = [this] { apply(); };
    outBox.onChange = [this] { apply(); };
    srBox.onChange  = [this] { apply(); };
    bufBox.onChange = [this] { apply(); };

    engine.getDeviceManager().addChangeListener (this);
    refresh();
}

DevicePanel::~DevicePanel()
{
    engine.getDeviceManager().removeChangeListener (this);
}

void DevicePanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();   // Gerät extern geändert (z. B. ab-/angesteckt) -> Combos spiegeln
}

void DevicePanel::refresh()
{
    updating = true;   // verhindert, dass das Befüllen apply() auslöst

    inBox.clear (juce::dontSendNotification);
    outBox.clear (juce::dontSendNotification);
    srBox.clear (juce::dontSendNotification);
    bufBox.clear (juce::dontSendNotification);
    sampleRates.clear();
    bufferSizes.clear();

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

    if (auto* dev = dm.getCurrentAudioDevice())
    {
        for (auto rate : dev->getAvailableSampleRates()) sampleRates.add (rate);
        for (int i = 0; i < sampleRates.size(); ++i)
            srBox.addItem (juce::String ((int) sampleRates[i]) + " Hz", i + 1);
        const double curSr = dev->getCurrentSampleRate();
        for (int i = 0; i < sampleRates.size(); ++i)
            if (std::abs (sampleRates[i] - curSr) < 1.0) srBox.setSelectedId (i + 1, juce::dontSendNotification);

        for (auto size : dev->getAvailableBufferSizes()) bufferSizes.add (size);
        for (int i = 0; i < bufferSizes.size(); ++i)
            bufBox.addItem (juce::String (bufferSizes[i]), i + 1);
        const int curBuf = dev->getCurrentBufferSizeSamples();
        for (int i = 0; i < bufferSizes.size(); ++i)
            if (bufferSizes[i] == curBuf) bufBox.setSelectedId (i + 1, juce::dontSendNotification);
    }

    srBox.setEnabled  (! sampleRates.isEmpty());
    bufBox.setEnabled (! bufferSizes.isEmpty());

    updating = false;
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

    double sr = 0.0;
    const int sid = srBox.getSelectedId();
    if (sid >= 1 && sid <= sampleRates.size()) sr = sampleRates[sid - 1];

    int buf = 0;
    const int bid = bufBox.getSelectedId();
    if (bid >= 1 && bid <= bufferSizes.size()) buf = bufferSizes[bid - 1];

    engine.setDeviceConfig (input, output, sr, buf);
}

void DevicePanel::toggleAdvanced()
{
    advancedVisible = advancedBtn.getToggleState();
    srLabel.setVisible (advancedVisible);
    srBox.setVisible (advancedVisible);
    bufLabel.setVisible (advancedVisible);
    bufBox.setVisible (advancedVisible);
    resized();
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

    // Label + "i"-Icon in der Label-Spalte, danach die ComboBox.
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

    advancedBtn.setBounds (row().removeFromLeft (120));

    if (advancedVisible)
    {
        auto srRow = row();
        srLabel.setBounds (srRow.removeFromLeft (labelW));   srBox.setBounds (srRow);
        auto bufRow = row();
        bufLabel.setBounds (bufRow.removeFromLeft (labelW)); bufBox.setBounds (bufRow);
    }
}
