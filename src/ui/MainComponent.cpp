#include "ui/MainComponent.h"
#include "audio/PluginChain.h"

namespace
{
    juce::String howToText()
    {
        return
            "How to use MicVST\n"
            "\n"
            "1)  Install a virtual audio cable\n"
            "    VB-Cable (free) is recommended (link below). MicVST detects it automatically.\n"
            "\n"
            "2)  Pick your devices (top of the window)\n"
            "    Input   =  your microphone\n"
            "    Output  =  the virtual cable (e.g. \"CABLE Input\")\n"
            "\n"
            "3)  Build your plugin chain  (\"+ Plugin\")\n"
            "    - Add VST3 effects (EQ, de-noise, compressor, ...).\n"
            "    - Insert \"Mono -> Stereo\" where you want stereo (before it the chain runs\n"
            "      mono = less CPU). \"Stereo -> Mono\" is available too.\n"
            "    - Drag the handle on the left to reorder; the trash icon removes (with a prompt).\n"
            "    - Double-click a plugin to open its editor.\n"
            "    - \"Manage custom VST3 Folders\" adds plugins from other locations.\n"
            "\n"
            "4)  Use it in any app\n"
            "    In Discord / OBS / Zoom / ..., select \"CABLE Output\" as the microphone.\n"
            "\n"
            "5)  That's it\n"
            "    Settings are saved automatically. Closing the window keeps MicVST running in the\n"
            "    tray (right-click the tray icon for options). Enable \"Run at startup\" to launch\n"
            "    it silently on boot.\n"
            "\n"
            "Auto-Update-Check\n"
            "    With \"Auto-Update-Check\" enabled, MicVST asks GitHub once on each start whether a\n"
            "    newer version exists. No data is collected and there is no auto-installer - if an\n"
            "    update is found, the version number turns into a link to the download.";
    }

    const char* const kRepoUrl = "https://github.com/philipz794/MicVST";

    // Inhalt des How-To-Fensters: Anleitungstext + klickbare Links (VB-Cable, GitHub).
    class HowToContent : public juce::Component
    {
    public:
        HowToContent()
        {
            body.setMultiLine (true);
            body.setReadOnly (true);
            body.setCaretVisible (false);
            body.setScrollbarsShown (true);
            body.setPopupMenuEnabled (false);
            body.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1e1e1e));
            body.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
            body.setColour (juce::TextEditor::textColourId, juce::Colours::white);
            body.setFont (juce::Font (juce::FontOptions (15.0f)));
            body.setText (howToText(), false);
            addAndMakeVisible (body);

            setupLink (vbCable, "Download VB-Cable  (vb-audio.com/Cable)", "https://vb-audio.com/Cable/");
            setupLink (github,  "Check for updates on GitHub", kRepoUrl);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (12);
            github.setBounds (r.removeFromBottom (22));
            r.removeFromBottom (6);
            vbCable.setBounds (r.removeFromBottom (22));
            r.removeFromBottom (10);
            body.setBounds (r);
        }

    private:
        void setupLink (juce::HyperlinkButton& b, const juce::String& text, const juce::String& url)
        {
            b.setButtonText (text);
            b.setURL (juce::URL (url));
            b.setJustificationType (juce::Justification::centredLeft);
            b.setColour (juce::HyperlinkButton::textColourId, juce::Colours::orange);
            addAndMakeVisible (b);
        }

        juce::TextEditor body;
        juce::HyperlinkButton vbCable, github;
    };

    // Eigenes Fenster mit der Anleitung. Schließen versteckt nur (Instanz bleibt zum Wieder-Öffnen).
    class HowToWindow : public juce::DocumentWindow
    {
    public:
        HowToWindow()
            : juce::DocumentWindow ("MicVST - How To", juce::Colours::darkgrey,
                                    juce::DocumentWindow::closeButton)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new HowToContent(), false);
            setResizable (true, false);
            centreWithSize (560, 560);
        }
        void closeButtonPressed() override { setVisible (false); }
    };
}

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

    addAndMakeVisible (howToBtn);
    howToBtn.setTooltip ("Quick setup guide");
    howToBtn.onClick = [this] { showHowTo(); };

    currentVersion = juce::JUCEApplication::getInstance()->getApplicationVersion();
    versionLink.setButtonText ("v" + currentVersion);
    versionLink.setURL (juce::URL (kRepoUrl));
    versionLink.setTooltip ("MicVST on GitHub");
    versionLink.setJustificationType (juce::Justification::centredLeft);
    versionLink.setColour (juce::HyperlinkButton::textColourId, juce::Colours::grey);
    addAndMakeVisible (versionLink);

    updateToggle.setTooltip ("Check GitHub on startup whether a newer version exists (opt-in, no data collected)");
    updateToggle.onClick = [this]
    {
        const bool on = updateToggle.getToggleState();
        if (onUpdateCheckToggled) onUpdateCheckToggled (on);
        if (on) startUpdateCheck();
    };
    addAndMakeVisible (updateToggle);

    addAndMakeVisible (autostartToggle);
    autostartToggle.setTooltip ("Launch MicVST silently into the tray when Windows starts");
    autostartToggle.setToggleState (AutostartRegistry::isEnabled(), juce::dontSendNotification);
    autostartToggle.onClick = [this] { AutostartRegistry::setEnabled (autostartToggle.getToggleState()); };

    cableHint.setColour (juce::HyperlinkButton::textColourId, juce::Colours::orange);
    cableHint.setJustificationType (juce::Justification::centredLeft);
    cableHint.setTooltip ("https://vb-audio.com/Cable/");
    addChildComponent (cableHint);   // Sichtbarkeit steuert updateCableHint()

    engine.onStatusChanged = [this] { updateCableHint(); };
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
    // Mit der Registry synchron halten (z. B. wenn der Tray den Autostart umschaltet).
    autostartToggle.setToggleState (AutostartRegistry::isEnabled(), juce::dontSendNotification);
}

void MainComponent::showHowTo()
{
    if (howToWindow == nullptr)
        howToWindow.reset (new HowToWindow());
    howToWindow->setVisible (true);
    howToWindow->toFront (true);
}

void MainComponent::setUpdateCheckEnabled (bool on, bool runIfOn)
{
    updateToggle.setToggleState (on, juce::dontSendNotification);
    if (on && runIfOn) startUpdateCheck();
}

void MainComponent::startUpdateCheck()
{
    // SafePointer: falls die Komponente vor dem (asynchronen) Ergebnis zerstört wird,
    // läuft der Callback ins Leere statt in einen Dangling-this.
    juce::Component::SafePointer<MainComponent> safe (this);
    updateChecker.start (currentVersion, [safe] (UpdateChecker::Result r)
    {
        if (auto* self = safe.getComponent())
        {
            self->showUpdateAvailable (r.latestVersion, r.releaseUrl);
            if (self->onUpdateFound) self->onUpdateFound (r.latestVersion, r.releaseUrl);
        }
    });
}

void MainComponent::showUpdateAvailable (const juce::String& latestVersion, const juce::String& url)
{
    versionLink.setButtonText ("v" + currentVersion + " > Update available!");
    versionLink.setURL (juce::URL (url));
    versionLink.setTooltip ("MicVST " + latestVersion + " is available on GitHub - click to open");
    versionLink.setColour (juce::HyperlinkButton::textColourId, juce::Colours::orange);
    resized();   // Text ist breiter -> Layout der unteren Leiste auffrischen
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
    auto bottomRow = r.removeFromBottom (24);
    // Rechts, von außen nach innen: Run at startup (Rand) | Auto-Update-Check | How To.
    autostartToggle.setBounds (bottomRow.removeFromRight (120));
    bottomRow.removeFromRight (6);
    updateToggle.setBounds (bottomRow.removeFromRight (150));
    bottomRow.removeFromRight (6);
    howToBtn.setBounds (bottomRow.removeFromRight (80));
    // Links die klickbare Version: Breite an den Text anpassen, sonst ist der leere Reserve-
    // Platz (für "> Update available!") mit-klickbar.
    versionLink.setBounds (bottomRow);
    versionLink.changeWidthToFitText();
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

    // DevicePanel feste Höhe (Input + Output + Info-Zeile = 3 Zeilen à 26 + 2 Lücken à 4),
    // die Plugin-Liste bekommt den ganzen Rest -> mehr Fensterhöhe verlängert die Liste.
    constexpr int devicePanelH = 86;
    devicePanel->setBounds (r.removeFromTop (devicePanelH));
    r.removeFromTop (8);
    pluginList->setBounds (r);
}
