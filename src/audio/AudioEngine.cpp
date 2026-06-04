#include "audio/AudioEngine.h"
using IOProc = juce::AudioProcessorGraph::AudioGraphIOProcessor;

AudioEngine::AudioEngine()
{
    // WICHTIG: erzwingt das Erstellen + Scannen der Geräte-Typen. Ohne diesen Aufruf
    // ist availableDeviceTypes leer, getCurrentDeviceTypeObject() liefert nullptr und
    // setCurrentAudioDeviceType()/setAudioDeviceSetup() sowie die Device-Suche tun nichts.
    deviceManager.getAvailableDeviceTypes();
    deviceManager.addChangeListener (this);
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeChangeListener (this);
    deviceManager.removeAudioCallback (this);
    deviceManager.closeAudioDevice();
    player.setProcessor (nullptr);
}

bool AudioEngine::isRunning() const
{
    auto* dev = deviceManager.getCurrentAudioDevice();
    return dev != nullptr && dev->isPlaying();
}

void AudioEngine::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Device kam/ging: AudioDeviceManager stellt das gespeicherte Setup selbst
    // wieder her (namensbasiert). Wir spiegeln nur den Status nach außen.
    juce::Logger::writeToLog (isRunning() ? "Audio: läuft"
                                          : "Audio: idle (Device getrennt?)");
    if (onStatusChanged) onStatusChanged();
    if (onDeviceChanged) onDeviceChanged();   // Geräte-Einstellungen persistieren
}

juce::String AudioEngine::detectCableOutput()
{
    // Render-Endpunkte bekannter virtueller Kabel, nach Priorität (VB-Cable zuerst).
    static const char* const cablePatterns[] = {
        "CABLE Input",          // VB-Audio Virtual Cable (empfohlen)
        "VB-Audio",             // weitere VB-Audio-Kabel (Hi-Fi Cable etc.)
        "VoiceMeeter Input",    // VoiceMeeter VAIO/Aux
        "Virtual Audio Cable"   // VAC ("Line 1 (Virtual Audio Cable)")
    };

    deviceManager.setCurrentAudioDeviceType ("Windows Audio", true);
    if (auto* type = deviceManager.getCurrentDeviceTypeObject())
    {
        type->scanForDevices();
        auto outs = type->getDeviceNames (false /* output */);
        for (auto* pat : cablePatterns)
            for (auto& name : outs)
                if (name.containsIgnoreCase (pat))
                    return name;
    }
    return {};
}

juce::String AudioEngine::initialise (const juce::String& inputDeviceName,
                                      const juce::String& outputDeviceName)
{
    deviceManager.removeAudioCallback (this);   // idempotent: doppelte Registrierung vermeiden
    deviceManager.closeAudioDevice();
    deviceManager.setCurrentAudioDeviceType ("Windows Audio", true);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    // Namen VERBATIM übernehmen — auch leer. Ein leerer Output-Name bedeutet bewusst
    // "kein Host-Output" ("none"); der Input-WASAPI-Callback treibt die Engine dann allein.
    // Würden wir leer überspringen, bliebe das alte Default-Gerät stehen und "none"
    // ließe sich nicht speichern.
    setup.inputDeviceName  = inputDeviceName;
    setup.outputDeviceName = outputDeviceName;
    setup.bufferSize = 128;
    // Kanäle EXPLIZIT aktivieren. Auf useDefault* darf man sich nicht verlassen:
    // ohne deviceManager.initialise(numIn,numOut,...) ist numInputChansNeeded=0,
    // wodurch die "Default"-Input-Kanäle auf [0,0) = KEINE gesetzt würden.
    setup.useDefaultInputChannels  = false;
    setup.useDefaultOutputChannels = false;
    setup.inputChannels.clear();
    setup.inputChannels.setRange (0, 2, true);    // bis zu 2 Mic-Kanäle (mono nutzt nur ch0)
    setup.outputChannels.clear();
    setup.outputChannels.setRange (0, 2, true);
    setup.sampleRate = 48000.0;   // bevorzugt; WASAPI shared kann die Mix-Rate des Geräts erzwingen

    // Fehler NICHT früh zurückgeben: Graph/Chain müssen immer existieren,
    // auch wenn (noch) kein Device offen ist (z. B. Gerät noch nicht da / Reconnect).
    const juce::String err = deviceManager.setAudioDeviceSetup (setup, true);

    if (auto* d = deviceManager.getCurrentAudioDevice())
        juce::Logger::writeToLog ("Setup: in=" + setup.inputDeviceName
            + " out=" + setup.outputDeviceName
            + " sr=" + juce::String (d->getCurrentSampleRate(), 0)
            + " buf=" + juce::String (d->getCurrentBufferSizeSamples()));
    else
        juce::Logger::writeToLog ("Setup: kein Device offen (" + err + ")");

    graph.clear();
    auto inNode  = graph.addNode (std::make_unique<IOProc> (IOProc::audioInputNode));
    auto outNode = graph.addNode (std::make_unique<IOProc> (IOProc::audioOutputNode));

    pluginChain = std::make_unique<PluginChain> (graph, inNode->nodeID, outNode->nodeID);
    rebuildGraph();   // leere Kette: in -> out (inkl. Mono→Stereo-Fanout)

    // Eigenen "spielenden" Playhead setzen, BEVOR der Player seinen (ohne isPlaying)
    // installiert. Der Player nutzt seinen nur, wenn der Graph keinen hat.
    if (auto* d = deviceManager.getCurrentAudioDevice())
        playHead.sampleRate.store (d->getCurrentSampleRate());
    graph.setPlayHead (&playHead);

    player.setProcessor (&graph);
    deviceManager.addAudioCallback (this);
    return err;
}

void AudioEngine::setDeviceConfig (const juce::String& input, const juce::String& output,
                                  double sampleRate, int bufferSize)
{
    auto setup = deviceManager.getAudioDeviceSetup();
    setup.inputDeviceName  = input;
    setup.outputDeviceName = output;
    if (sampleRate > 0.0) setup.sampleRate = sampleRate;
    if (bufferSize > 0)   setup.bufferSize = bufferSize;
    // Kanäle EXPLIZIT (wie in initialise) — sonst droht 0 aktive Input-Kanäle.
    setup.useDefaultInputChannels  = false;
    setup.useDefaultOutputChannels = false;
    setup.inputChannels.clear();  setup.inputChannels.setRange (0, 2, true);
    setup.outputChannels.clear(); setup.outputChannels.setRange (0, 2, true);

    deviceManager.setAudioDeviceSetup (setup, true);
    rebuildGraph();   // IO-Knoten-Kanalzahl kann sich geändert haben -> neu verdrahten
}

void AudioEngine::rebuildGraph()
{
    if (pluginChain != nullptr)
        pluginChain->rebuildConnections();
}

void AudioEngine::scanPlugins()
{
    knownPlugins.clear();   // autoritativ aus Standardordner + aktuellen Custom-Ordnern neu aufbauen
    PluginChain::scan (formatManager, knownPlugins, pluginFolders);
}

void AudioEngine::addPluginFolder (const juce::String& folder)
{
    if (folder.isNotEmpty() && ! pluginFolders.contains (folder))
        pluginFolders.add (folder);
    scanPlugins();
}

void AudioEngine::removePluginFolder (const juce::String& folder)
{
    pluginFolders.removeString (folder);
    scanPlugins();
}

MicVSTState AudioEngine::captureState()
{
    MicVSTState s;
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    s.inputDevice  = setup.inputDeviceName;
    s.outputDevice = setup.outputDeviceName;
    s.sampleRate   = setup.sampleRate;
    s.bufferSize   = setup.bufferSize > 0 ? setup.bufferSize : 128;
    s.pluginFolders = pluginFolders;
    if (pluginChain == nullptr) return s;

    for (auto& e : pluginChain->entries())
    {
        PluginEntryState p;
        p.fileOrId = e.fileOrId;
        p.bypassed = e.bypassed;
        if (auto* node = graph.getNodeForId (e.node))
        {
            auto* proc = node->getProcessor();
            const juce::ScopedLock sl (proc->getCallbackLock());   // gegen Race mit processBlock
            proc->getStateInformation (p.state);
        }
        s.plugins.add (p);
    }
    return s;
}

void AudioEngine::applyState (const MicVSTState& s)
{
    initialise (s.inputDevice, s.outputDevice);

    double sr = deviceManager.getCurrentAudioDevice() != nullptr
              ? deviceManager.getCurrentAudioDevice()->getCurrentSampleRate() : 48000.0;

    for (auto& p : s.plugins)
    {
        // Interne Glieder wiederherstellen.
        if (p.fileOrId == PluginChain::monoToStereoId || p.fileOrId == PluginChain::stereoToMonoId)
        {
            if (p.fileOrId == PluginChain::monoToStereoId) pluginChain->addMonoToStereo();
            else                                           pluginChain->addStereoToMono();
            pluginChain->setBypass ((int) pluginChain->entries().size() - 1, p.bypassed);
            continue;
        }

        auto type = knownPlugins.getTypeForFile (p.fileOrId);
        if (type == nullptr) { juce::Logger::writeToLog ("Plugin fehlt: " + p.fileOrId); continue; }
        juce::String err;
        if (pluginChain->addPlugin (formatManager, *type, sr, 128, err))
        {
            const int idx = (int) pluginChain->entries().size() - 1;
            if (auto* node = graph.getNodeForId (pluginChain->entries()[(size_t) idx].node))
                node->getProcessor()->setStateInformation (p.state.getData(), (int) p.state.getSize());
            pluginChain->setBypass (idx, p.bypassed);
        }
        else juce::Logger::writeToLog ("Plugin-Load: " + err);
    }
    rebuildGraph();
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    // Geöffnete Geräte-/Kanalkonfiguration ins Log (hilft beim Diagnostizieren von Audio-Problemen).
    juce::Logger::writeToLog ("Device start: '" + device->getName() + "'"
        + " | inCh aktiv=" + juce::String (device->getActiveInputChannels().countNumberOfSetBits())
        + " von [" + device->getInputChannelNames().joinIntoString (", ") + "]"
        + " | outCh aktiv=" + juce::String (device->getActiveOutputChannels().countNumberOfSetBits())
        + " | sr=" + juce::String (device->getCurrentSampleRate(), 0)
        + " | buf=" + juce::String (device->getCurrentBufferSizeSamples()));
    player.audioDeviceAboutToStart (device);
}

void AudioEngine::audioDeviceStopped()
{
    player.audioDeviceStopped();
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext& context)
{
    playHead.samples.fetch_add (numSamples, std::memory_order_relaxed);   // Transport voranschieben

    if (numInputChannels > 0)
    {
        juce::AudioBuffer<float> inView (const_cast<float* const*> (inputChannelData),
                                         numInputChannels, numSamples);
        inputMeter.process (inView);
    }

    // Graph verarbeiten (Input -> Kette -> Output).
    player.audioDeviceIOCallbackWithContext (inputChannelData, numInputChannels,
                                             outputChannelData, numOutputChannels,
                                             numSamples, context);

    if (numOutputChannels > 0)
    {
        juce::AudioBuffer<float> outView (outputChannelData, numOutputChannels, numSamples);
        outputMeter.process (outView);
    }
}
