#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

struct LevelReading { float rms = 0.0f; float peak = 0.0f; };

// Reine Funktion: RMS/Peak über alle Kanäle eines Buffers. Testbar ohne Hardware.
LevelReading computeLevel (const juce::AudioBuffer<float>& buffer);

// Thread-sicherer Meter: process() im Audio-Thread, read() im GUI-Thread.
class LevelMeter
{
public:
    void process (const juce::AudioBuffer<float>& buffer)
    {
        const auto r = computeLevel (buffer);
        rms_.store (r.rms,  std::memory_order_relaxed);
        peak_.store (r.peak, std::memory_order_relaxed);
    }
    LevelReading read() const
    {
        return { rms_.load (std::memory_order_relaxed),
                 peak_.load (std::memory_order_relaxed) };
    }
private:
    std::atomic<float> rms_  { 0.0f };
    std::atomic<float> peak_ { 0.0f };
};
