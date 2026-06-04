#include "state/AutostartRegistry.h"

namespace
{
    const juce::String keyPath =
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\MicVST";
}

namespace AutostartRegistry
{
    bool isEnabled()
    {
        return juce::WindowsRegistry::valueExists (keyPath);
    }

    void setEnabled (bool shouldRun)
    {
        if (shouldRun)
        {
            auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                          .getFullPathName();
            juce::WindowsRegistry::setValue (keyPath, "\"" + exe + "\" --tray");
        }
        else
        {
            juce::WindowsRegistry::deleteValue (keyPath);
        }
    }
}
