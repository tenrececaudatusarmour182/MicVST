#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>   // AudioProcessorPlayer lebt in juce_audio_utils
#include "audio/Metering.h"
#include "audio/PluginChain.h"
#include "audio/MicVSTDeviceManager.h"
#include "state/Persistence.h"

// Besitzt AudioDeviceManager + AudioProcessorGraph. Der Graph läuft über einen
// internen AudioProcessorPlayer; AudioEngine bleibt der Device-Callback und
// metert Input/Output rund um den Player herum.
class AudioEngine : private juce::AudioIODeviceCallback,
                    private juce::ChangeListener
{
public:
    AudioEngine();
    ~AudioEngine() override;

    juce::String initialise (const juce::String& inputDeviceName,
                             const juce::String& outputDeviceName);

    // Laufzeit-Geräteumschaltung (vom DevicePanel): setzt Geräte/Samplerate/Buffer neu,
    // OHNE Graph + Plugin-Kette neu aufzubauen. sampleRate<=0 / bufferSize<=0 = unverändert.
    void setDeviceConfig (const juce::String& input, const juce::String& output,
                          double sampleRate, int bufferSize);

    // Sucht ein installiertes virtuelles Audio-Kabel als Output (VB-Cable, VoiceMeeter, VAC),
    // die Render->Capture selbst spiegeln. Leerer String = kein Kabel gefunden.
    juce::String detectCableOutput();

    MicVSTState captureState();            // liest Devices + Plugin-Kette + Blobs
    void          applyState (const MicVSTState&);   // lädt Devices + Plugins + setStateInformation

    bool isRunning() const;                 // true wenn ein Audio-Device offen ist und spielt
    std::function<void()> onStatusChanged;  // wird bei Device-Änderungen aufgerufen (UI-Status)
    std::function<void()> onDeviceChanged;  // wird bei Geräte-Änderungen aufgerufen (zum Persistieren)

    juce::AudioDeviceManager&       getDeviceManager() { return deviceManager; }
    juce::AudioProcessorGraph&      getGraph()         { return graph; }
    PluginChain&                    getChain()         { return *pluginChain; }
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }
    juce::KnownPluginList&          getKnownPlugins()  { return knownPlugins; }

    // VST3-Scan: Standardordner + zusätzliche Custom-Ordner. add/remove rescannen jeweils.
    void scanPlugins();   // baut die KnownPluginList komplett neu auf
    void addPluginFolder (const juce::String& folder);
    void removePluginFolder (const juce::String& folder);
    void setPluginFolders (const juce::StringArray& f) { pluginFolders = f; }
    const juce::StringArray& getPluginFolders() const  { return pluginFolders; }

    LevelReading inputLevel()  const { return inputMeter.read(); }
    LevelReading outputLevel() const { return outputMeter.read(); }

    void rebuildGraph();   // Graph-Verbindungen neu aufbauen (inkl. Mono->Stereo-Fanout)

private:
    // Playhead, der dem Graph (und damit allen Plugins) durchgehend "Transport läuft"
    // meldet. Nötig, weil der Default-Playhead des AudioProcessorPlayer isPlaying NICHT
    // setzt — manche Routing/Streaming-Plugins senden aber nur bei laufendem Transport.
    struct PlayingHead : juce::AudioPlayHead
    {
        std::atomic<juce::int64> samples { 0 };
        std::atomic<double> sampleRate { 48000.0 };
        juce::Optional<PositionInfo> getPosition() const override
        {
            const auto s  = samples.load (std::memory_order_relaxed);
            const auto sr = sampleRate.load (std::memory_order_relaxed);
            PositionInfo info;
            info.setIsPlaying (true);
            info.setIsRecording (false);
            info.setIsLooping (false);
            info.setTimeInSamples (s);
            info.setTimeInSeconds ((double) s / sr);
            info.setBpm (120.0);
            info.setTimeSignature (juce::AudioPlayHead::TimeSignature{});
            info.setPpqPosition (((double) s / sr) * (120.0 / 60.0));
            return info;
        }
    };
    PlayingHead playHead;

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    MicVSTDeviceManager deviceManager;   // nur WASAPI-Shared -> keine Typ-Auswahl im Selector
    juce::AudioProcessorGraph graph;
    juce::AudioProcessorPlayer player;
    std::unique_ptr<PluginChain> pluginChain;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    juce::StringArray pluginFolders;   // zusätzliche VST3-Suchordner (persistiert)
    LevelMeter inputMeter, outputMeter;
};
