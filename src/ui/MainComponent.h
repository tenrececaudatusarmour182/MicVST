#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/AudioEngine.h"
#include "ui/LevelMeterComponent.h"
#include "ui/PluginListView.h"
#include "ui/DevicePanel.h"
#include "state/AutostartRegistry.h"

class MainComponent : public juce::Component, private juce::Timer
{
public:
    explicit MainComponent (AudioEngine& engine);
    ~MainComponent() override;
    void resized() override;
private:
    void timerCallback() override;
    void updateStatus();
    void updateCableHint();   // zeigt den VB-Cable-Hinweis, wenn kein virtuelles Kabel installiert ist
    AudioEngine& engine;
    std::unique_ptr<DevicePanel> devicePanel;
    LevelMeterComponent inMeter, outMeter;
    DbScaleComponent dbScale;
    juce::Label inLabel { {}, "In" }, outLabel { {}, "Out" };
    std::unique_ptr<PluginListView> pluginList;
    juce::Label statusLabel;
    juce::ToggleButton autostartToggle { "Run at startup" };
    juce::HyperlinkButton cableHint { "No virtual audio cable found - click to install VB-Cable",
                                      juce::URL ("https://vb-audio.com/Cable/") };
    juce::TooltipWindow tooltipWindow { nullptr, 100 };   // Hover-Tooltips, fast instant (100 ms)
};
