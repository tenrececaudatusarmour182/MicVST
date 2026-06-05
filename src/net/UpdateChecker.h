#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>   // MessageManager::callAsync

// Fragt EINMAL die GitHub-"latest release"-API ab und vergleicht die Version mit der
// laufenden. Reine Netzwerk-/Vergleichslogik, sonst nichts: das Ergebnis kommt per
// MessageManager::callAsync auf den Message-Thread zurück.
//
// Owned vom Aufrufer und im Destructor gestoppt -> kein Lifetime-Problem beim Beenden.
// Kein curl nötig: Windows nutzt WinINet auch bei JUCE_USE_CURL=0.
class UpdateChecker : private juce::Thread
{
public:
    struct Result
    {
        bool         updateAvailable = false;
        juce::String latestVersion;   // ohne führendes "v", z. B. "1.0.3"
        juce::String releaseUrl;      // Releases-Seite (für den Klick)
    };

    UpdateChecker() : juce::Thread ("UpdateChecker") {}
    ~UpdateChecker() override { stopThread (4000); }

    // Startet einen Check (no-op, wenn schon einer läuft). onDone wird auf dem Message-Thread
    // aufgerufen, NUR wenn ein Update gefunden wurde (kein Callback bei "aktuell"/Fehler/offline).
    void start (const juce::String& currentVersion, std::function<void (Result)> onDone)
    {
        if (isThreadRunning()) return;
        current  = currentVersion;
        callback = std::move (onDone);
        startThread();
    }

    // Vergleicht zwei "x.y.z"-Versionen (führendes "v" wird ignoriert). true, wenn a > b.
    static bool isNewer (const juce::String& a, const juce::String& b)
    {
        auto parts = [] (juce::String s)
        {
            s = s.trim();
            if (s.startsWithIgnoreCase ("v")) s = s.substring (1);
            juce::StringArray t;
            t.addTokens (s, ".", {});
            return t;
        };
        const auto pa = parts (a), pb = parts (b);
        const int n = juce::jmax (pa.size(), pb.size());
        for (int i = 0; i < n; ++i)
        {
            const int va = i < pa.size() ? pa[i].getIntValue() : 0;
            const int vb = i < pb.size() ? pb[i].getIntValue() : 0;
            if (va != vb) return va > vb;
        }
        return false;
    }

private:
    void run() override
    {
        Result r;

        const juce::URL url ("https://api.github.com/repos/philipz794/MicVST/releases/latest");
        // GitHub verlangt einen User-Agent, sonst 403. Accept-Header wie von der API empfohlen.
        const auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                              .withExtraHeaders ("User-Agent: MicVST\r\n"
                                                 "Accept: application/vnd.github+json\r\n")
                              .withConnectionTimeoutMs (4000);

        std::unique_ptr<juce::InputStream> stream (url.createInputStream (opts));
        if (stream == nullptr || threadShouldExit())
            return;   // offline / Fehler / Abbruch -> still nichts melden

        const auto json = juce::JSON::parse (stream->readEntireStreamAsString());
        if (threadShouldExit())
            return;

        const auto tag = json.getProperty ("tag_name", juce::var()).toString();
        if (tag.isEmpty() || ! isNewer (tag, current))
            return;   // keine/aktuelle Version -> kein Callback

        r.updateAvailable = true;
        r.latestVersion   = tag.startsWithIgnoreCase ("v") ? tag.substring (1) : tag;
        // html_url zeigt direkt auf die Release-Seite; Fallback auf /releases/latest.
        r.releaseUrl = json.getProperty ("html_url",
                          "https://github.com/philipz794/MicVST/releases/latest").toString();

        auto cb = callback;
        juce::MessageManager::callAsync ([cb, r] { if (cb) cb (r); });
    }

    juce::String current;
    std::function<void (Result)> callback;
};
