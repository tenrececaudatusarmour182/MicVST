#pragma once
#include <juce_audio_devices/juce_audio_devices.h>

// AudioDeviceManager, der NUR den WASAPI-Shared-Typ registriert.
// Default-JUCE registriert drei Windows-Audio-Typen (shared, exclusive,
// sharedLowLatency) -> der AudioDeviceSelectorComponent zeigt dann eine
// Typ-Auswahl. Wir brauchen nur "Windows Audio" (shared); mit genau einem
// Typ blendet der Selector die Typ-Zeile aus.
struct MicVSTDeviceManager : juce::AudioDeviceManager
{
    void createAudioDeviceTypes (juce::OwnedArray<juce::AudioIODeviceType>& types) override
    {
        if (auto* wasapi = juce::AudioIODeviceType::createAudioIODeviceType_WASAPI (
                               juce::WASAPIDeviceMode::shared))
            types.add (wasapi);
    }
};
