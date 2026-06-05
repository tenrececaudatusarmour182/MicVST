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
    const juce::String getApplicationVersion() override { return "1.0.2"; }
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

        // Auto-Update-Check-Zustand übernehmen (Default: aus, nie gefragt).
        updateCheckEnabled  = state.updateCheckEnabled;
        updateCheckAsked    = state.updateCheckAsked;
        lastNotifiedVersion = state.lastNotifiedVersion;

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

        mainWindow = std::make_unique<MainWindow> ("MicVST", *engine, state.windowState);
        mainWindow->setVisible (! silent);   // Autostart (--tray): unsichtbar, nur Tray

        tray = std::make_unique<TrayIcon>();
        tray->onToggleWindow = [this] { toggleWindow(); };
        tray->onQuit         = [this] { systemRequestedQuit(); };
        tray->isAutostartOn  = [] { return AutostartRegistry::isEnabled(); };
        tray->setAutostart   = [] (bool on) { AutostartRegistry::setEnabled (on); };

        // Beim Verstecken: einmaligen Tray-Hinweis zeigen und die aktuelle Fenstergröße sichern
        // (X beendet nicht, daher hier persistieren, damit eine geänderte Größe nicht verloren geht).
        mainWindow->onHide = [this] { maybeShowTrayHint(); persistState(); };

        // Erst JETZT (nach applyState) das Persistieren bei Geräte-Änderungen aktivieren,
        // damit In-/Output-Auswahl gemerkt wird — auch ohne sauberes Beenden (X versteckt nur).
        engine->onDeviceChanged = [this] { persistState(); };

        // --- Auto-Update-Check verdrahten ---
        if (auto* mc = mainWindow->getContent())
        {
            mc->onUpdateCheckToggled = [this] (bool on) { updateCheckEnabled = on; persistState(); };
            mc->onUpdateFound = [this] (const juce::String& latestVersion, const juce::String&)
            {
                // Tray-Bubble nur EINMAL pro neuer Version (der Fenster-Hinweis bleibt sowieso).
                if (latestVersion == lastNotifiedVersion) return;
                lastNotifiedVersion = latestVersion;
                persistState();
                if (tray != nullptr)
                    tray->showInfoBubble ("Update available",
                        "MicVST " + latestVersion + " is available - click the version number to update.");
            };
            // Bei aktivem Check sofort einen Lauf starten (auch im stillen Autostart -> Tray-Bubble).
            mc->setUpdateCheckEnabled (updateCheckEnabled, true);
        }

        // Erststart: einmalig nach Zustimmung fragen (nur mit sichtbarem Fenster).
        if (! silent && ! updateCheckAsked)
            askUpdateConsent();
    }

    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow != nullptr) { mainWindow->setVisible (true); mainWindow->toFront (true); }
    }

    void systemRequestedQuit() override
    {
        persistState();
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
    // Engine-Zustand (Geräte/Plugins) + Fenstergröße/-position + Update-Check-Zustand speichern.
    void persistState()
    {
        if (engine == nullptr) return;
        auto s = engine->captureState();
        if (mainWindow != nullptr) s.windowState = mainWindow->getWindowStateAsString();
        s.updateCheckEnabled  = updateCheckEnabled;
        s.updateCheckAsked    = updateCheckAsked;
        s.lastNotifiedVersion = lastNotifiedVersion;
        saveState (s);
    }

    // Einmaliges Erststart-Popup: Zustimmung zum Update-Check einholen. Egal wie der Nutzer
    // entscheidet -> "gefragt" wird gemerkt, also kommt der Dialog nie wieder.
    void askUpdateConsent()
    {
        updateCheckAsked = true;
        persistState();
        juce::NativeMessageBox::showYesNoBox (
            juce::MessageBoxIconType::QuestionIcon,
            "Check for updates?",
            "Should MicVST check GitHub for a newer version on startup?\n\n"
            "Only a single request is sent to GitHub - no data is collected and there is no "
            "auto-installer. You can change this anytime with the \"Auto-Update-Check\" box.",
            nullptr,
            juce::ModalCallbackFunction::create ([this] (int result)
            {
                const bool yes = (result == 1);
                updateCheckEnabled = yes;
                persistState();
                if (mainWindow != nullptr)
                    if (auto* mc = mainWindow->getContent())
                        mc->setUpdateCheckEnabled (yes, true);   // bei Ja: gleich prüfen
            }));
    }

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
        MainWindow (juce::String name, AudioEngine& engine, const juce::String& windowState)
            : DocumentWindow (name, juce::Colours::darkgrey, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            // resize-to-fit AUS: die Fenstergröße diktiert centreWithSize unten, nicht die
            // Content-Größe. Sonst zieht der MainComponent (setSize im Ctor) die Startgröße
            // über das Minimum hinaus.
            content = new MainComponent (engine);
            setContentOwned (content, false);
            setResizable (true, false);
            // Mindestgröße = Startgröße (600x480); zusätzliche Höhe verlängert die Plugin-Liste.
            setResizeLimits (600, 480, 1600, 1400);
            // Letzte Größe/Position wiederherstellen (innerhalb der Resize-Limits), sonst zentriert
            // auf Startgröße. restoreWindowStateFromString respektiert die gesetzten Limits.
            if (windowState.isNotEmpty())
                restoreWindowStateFromString (windowState);
            else
                centreWithSize (600, 480);
        }
        std::function<void()> onHide;
        void closeButtonPressed() override { setVisible (false); if (onHide) onHide(); }
        MainComponent* getContent() const { return content; }
    private:
        MainComponent* content = nullptr;   // gehört dem Fenster (setContentOwned), nur Zugriffszeiger
    };

    std::unique_ptr<juce::FileLogger> logger;
    std::unique_ptr<AudioEngine> engine;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<TrayIcon> tray;

    // Auto-Update-Check-Zustand (in config.xml persistiert, siehe persistState()).
    bool updateCheckEnabled = false;
    bool updateCheckAsked   = false;
    juce::String lastNotifiedVersion;
};

START_JUCE_APPLICATION (MicVSTApplication)
