#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>

// Tray-Icon mit Rechtsklick-Menü. Aktionen werden via std::function injiziert,
// damit der Tray nichts über App-Interna weiß.
class TrayIcon : public juce::SystemTrayIconComponent,
                public juce::DarkModeSettingListener
{
public:
    TrayIcon();
    ~TrayIcon() override;

    std::function<void()>         onToggleWindow;
    std::function<void()>         onQuit;

    std::function<bool()>         isAutostartOn;
    std::function<void(bool)>     setAutostart;

    void mouseDown (const juce::MouseEvent&) override;

    // Tray-Icon passend zum aktuellen Windows-Hell/Dunkel-Modus setzen.
    void updateIcon();
    void darkModeSettingChanged() override { updateIcon(); }   // Live-Umschaltung
};
