#include <juce_gui_basics/juce_gui_basics.h>
#include "audio/AudioEngine.h"
#include "state/Persistence.h"
#include "state/AutostartRegistry.h"
#include "ui/MainComponent.h"
#include "ui/TrayIcon.h"

class MicVSTApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "MicVST"; }
    const juce::String getApplicationVersion() override { return "1.0.1"; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String& commandLine) override
    {
        auto logFile = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile ("MicVST").getChildFile ("log.txt");
        logFile.getParentDirectory().createDirectory();
        logger.reset (new juce::FileLogger (logFile, "MicVST Log"));
        juce::Logger::setCurrentLogger (logger.get());

        const bool silent = commandLine.contains ("--tray");

        engine = std::make_unique<AudioEngine>();

        // Gespeicherten Zustand laden; NUR beim allerersten Start (noch keine config.xml)
        // Devices vorbelegen. Sobald eine Config existiert, gilt sie verbatim — sonst würde
        // eine bewusst auf "none" gesetzte Output-Auswahl beim Neustart wieder überschrieben.
        const bool firstRun = ! configFile().existsAsFile();
        MicVSTState state = loadState();

        // Custom-VST3-Ordner übernehmen, dann scannen (vor applyState und vor der UI).
        engine->setPluginFolders (state.pluginFolders);
        engine->scanPlugins();
        if (firstRun)
        {
            // Output bevorzugt ein virtuelles Kabel (VB-Cable & Co): die App liefert dann
            // direkt an ein virtuelles Mikro, ohne zusätzliches Ausgabe-Plugin. Kein Kabel
            // gefunden -> Output bleibt leer ("none"), die UI weist auf den Download hin.
            state.outputDevice = engine->detectCableOutput();
            juce::Logger::writeToLog (state.outputDevice.isEmpty()
                ? "No virtual cable found -> output 'none'"
                : "Host output (cable) = " + state.outputDevice);

            // Input: System-Standardmikrofon, sonst erstes verfügbares Eingangsgerät.
            auto& dm = engine->getDeviceManager();
            dm.setCurrentAudioDeviceType ("Windows Audio", true);
            if (auto* type = dm.getCurrentDeviceTypeObject())
            {
                type->scanForDevices();
                auto ins = type->getDeviceNames (true);
                const int def = type->getDefaultDeviceIndex (true);
                state.inputDevice = juce::isPositiveAndBelow (def, ins.size()) ? ins[def]
                                  : (ins.isEmpty() ? juce::String() : ins[0]);
            }
            juce::Logger::writeToLog ("Input = " + state.inputDevice);
        }
        engine->applyState (state);

        mainWindow = std::make_unique<MainWindow> ("MicVST", *engine);
        mainWindow->setVisible (! silent);   // Autostart (--tray): unsichtbar, nur Tray

        tray = std::make_unique<TrayIcon>();
        tray->onToggleWindow = [this] { toggleWindow(); };
        tray->onQuit         = [this] { systemRequestedQuit(); };
        tray->isAutostartOn  = [] { return AutostartRegistry::isEnabled(); };
        tray->setAutostart   = [] (bool on) { AutostartRegistry::setEnabled (on); };

        // Beim ersten Verstecken einmalig erklären, dass die App im Tray weiterläuft.
        mainWindow->onHide = [this] { maybeShowTrayHint(); };

        // Erst JETZT (nach applyState) das Persistieren bei Geräte-Änderungen aktivieren,
        // damit In-/Output-Auswahl gemerkt wird — auch ohne sauberes Beenden (X versteckt nur).
        engine->onDeviceChanged = [this] { if (engine != nullptr) saveState (engine->captureState()); };
    }

    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow != nullptr) { mainWindow->setVisible (true); mainWindow->toFront (true); }
    }

    void systemRequestedQuit() override
    {
        if (engine != nullptr) saveState (engine->captureState());
        quit();
    }

    void shutdown() override
    {
        tray = nullptr;
        mainWindow = nullptr;
        engine = nullptr;
        juce::Logger::setCurrentLogger (nullptr);
        logger = nullptr;
    }

private:
    void toggleWindow()
    {
        if (mainWindow == nullptr) return;
        const bool show = ! mainWindow->isVisible();
        mainWindow->setVisible (show);
        if (show) mainWindow->toFront (true);
        else      maybeShowTrayHint();
    }

    // Einmaliger Tray-Hinweis (Marker in %APPDATA%\MicVST), damit Nutzer nicht denken,
    // sie hätten die App beendet, wenn das Fenster verschwindet.
    void maybeShowTrayHint()
    {
        if (tray == nullptr) return;
        auto marker = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                        .getChildFile ("MicVST").getChildFile ("tray_hint_shown");
        if (marker.existsAsFile()) return;
        marker.create();
        tray->showInfoBubble ("MicVST", "Still running in the tray - right-click the icon for options.");
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name, AudioEngine& engine)
            : DocumentWindow (name, juce::Colours::darkgrey, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent (engine), true);
            setResizable (true, false);
            // Mindestgröße = Startgröße: kleiner als 600x700 wird abgeschnitten, daher nur
            // Vergrößern erlauben.
            setResizeLimits (600, 700, 1600, 1400);
            centreWithSize (600, 700);
        }
        std::function<void()> onHide;
        void closeButtonPressed() override { setVisible (false); if (onHide) onHide(); }
    };

    std::unique_ptr<juce::FileLogger> logger;
    std::unique_ptr<AudioEngine> engine;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<TrayIcon> tray;
};

START_JUCE_APPLICATION (MicVSTApplication)
