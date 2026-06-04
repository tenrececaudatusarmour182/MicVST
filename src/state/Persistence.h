#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include <juce_core/juce_core.h>

struct PluginEntryState
{
    juce::String fileOrId;
    bool bypassed = false;
    juce::MemoryBlock state;   // getStateInformation()-Blob
};

struct MicVSTState
{
    juce::String inputDevice, outputDevice;
    double sampleRate = 48000.0;
    int    bufferSize = 128;
    juce::Array<PluginEntryState> plugins;
    juce::StringArray pluginFolders;   // zusätzliche VST3-Suchordner
};

juce::ValueTree  toValueTree (const MicVSTState&);
MicVSTState    fromValueTree (const juce::ValueTree&);

// Datei unter %APPDATA%\MicVST\config.xml
juce::File    configFile();
bool          saveState (const MicVSTState&);
MicVSTState loadState();
