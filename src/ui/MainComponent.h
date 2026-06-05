#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/AudioEngine.h"
#include "ui/LevelMeterComponent.h"
#include "ui/PluginListView.h"
#include "ui/DevicePanel.h"
#include "state/AutostartRegistry.h"
#include "net/UpdateChecker.h"

class MainComponent : public juce::Component, private juce::Timer
{
public:
    explicit MainComponent (AudioEngine& engine);
    ~MainComponent() override;
    void resized() override;

    // Checkbox-Zustand setzen, ohne onUpdateCheckToggled auszulösen (Init). runIfOn=true ->
    // bei aktivem Check sofort einen Lauf starten.
    void setUpdateCheckEnabled (bool on, bool runIfOn);

    std::function<void (bool)> onUpdateCheckToggled;   // User hat die Checkbox geklickt (App persistiert)
    // Update gefunden: App entscheidet über die Tray-Bubble (Dedup per Version) + persistiert.
    std::function<void (const juce::String& latestVersion, const juce::String& url)> onUpdateFound;

private:
    void timerCallback() override;
    void updateCableHint();   // zeigt den VB-Cable-Hinweis, wenn kein virtuelles Kabel installiert ist
    void showHowTo();         // öffnet das How-To-Fenster
    void startUpdateCheck();  // einen Hintergrund-Check anstoßen
    void showUpdateAvailable (const juce::String& latestVersion, const juce::String& url);
    AudioEngine& engine;
    std::unique_ptr<DevicePanel> devicePanel;
    LevelMeterComponent inMeter, outMeter;
    DbScaleComponent dbScale;
    juce::Label inLabel { {}, "In" }, outLabel { {}, "Out" };
    std::unique_ptr<PluginListView> pluginList;
    juce::HyperlinkButton versionLink;   // klickbare Versionsnummer -> GitHub-Repo (zeigt auch "Update available!")
    juce::String currentVersion;         // ohne "v", z. B. "1.0.2"
    juce::ToggleButton updateToggle { "Auto-Update-Check" };   // Opt-in GitHub-Check beim Start
    UpdateChecker updateChecker;
    juce::TextButton howToBtn { "How To" };
    std::unique_ptr<juce::DocumentWindow> howToWindow;
    juce::ToggleButton autostartToggle { "Run at startup" };
    juce::HyperlinkButton cableHint { "No virtual audio cable found - click to install VB-Cable",
                                      juce::URL ("https://vb-audio.com/Cable/") };
    juce::TooltipWindow tooltipWindow { nullptr, 100 };   // Hover-Tooltips, fast instant (100 ms)
};
