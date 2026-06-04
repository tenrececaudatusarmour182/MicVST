#include "audio/Metering.h"

LevelReading computeLevel (const juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();
    if (numCh == 0 || n == 0) return {};

    double sumSq = 0.0;
    float  peak  = 0.0f;
    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* d = buffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            const float s = d[i];
            sumSq += (double) s * s;
            peak = juce::jmax (peak, std::abs (s));
        }
    }
    const float rms = (float) std::sqrt (sumSq / (double) (numCh * n));
    return { rms, peak };
}
