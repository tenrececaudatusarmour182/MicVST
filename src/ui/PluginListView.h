#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/AudioEngine.h"

// Plugin-Kette als Liste. Pro Zeile (von links nach rechts):
//   - Drag-Handle (Punkte) zum Umsortieren per Drag&Drop,
//   - Plugin-Name (Doppelklick öffnet den Editor),
//   - Bypass-Schalter,
//   - Mülleimer-Icon zum Entfernen.
// Oben ein "+ Plugin"-Button (Dropdown aus KnownPluginList + internes Mono->Stereo).
class PluginListView : public juce::Component
{
public:
    explicit PluginListView (AudioEngine& engine);
    void resized() override;

private:
    static constexpr int rowH = 30;

    void showAddMenu();
    void showFolderMenu();              // Custom-VST3-Ordner verwalten (hinzufügen/entfernen)
    void chooseFolder();                // VST3-Ordner per Dialog wählen + rescannen
    void openEditor (int row);
    void rebuildRows();                 // Zeilen-Components aus der aktuellen Kette neu erzeugen
    void commitChange();                // rebuildGraph + persistieren + Zeilen aktualisieren
    void requestRemove (int index);     // verzögertes Entfernen (Row löscht sich sonst selbst)
    void requestMove (int from, int to);
    void toggleBypass (int index);

    // Per-Row gezeichneter Mülleimer-Button.
    struct TrashButton : juce::Button
    {
        TrashButton() : juce::Button ("Remove") {}
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    };

    // Eine Zeile: Handle + Name + Bypass + Trash. Das Handle-/Namensfeld behandelt
    // Drag (Umsortieren) und Doppelklick (Editor); Bypass/Trash sind Kind-Buttons.
    struct Row : juce::Component
    {
        Row (PluginListView& ownerIn, int idx);
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp   (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        bool overHandle (juce::Point<int> p) const;

        PluginListView& owner;
        int index;
        juce::TextButton bypassBtn;
        TrashButton trashBtn;
        bool dragging = false;
        int  grabOffsetY = 0;
    };

    // Editor-Fenster, das sich beim Schließen selbst aus der Liste entfernt.
    struct EditorWindow : juce::DocumentWindow
    {
        EditorWindow (const juce::String& name, std::function<void (EditorWindow*)> onCloseIn)
            : juce::DocumentWindow (name, juce::Colours::black, juce::DocumentWindow::closeButton),
              onClose (std::move (onCloseIn)) {}
        void closeButtonPressed() override { if (onClose) onClose (this); }
        std::function<void (EditorWindow*)> onClose;
    };

    AudioEngine& engine;
    juce::TextButton addBtn { "+ Plugin" };
    juce::TextButton manageFoldersBtn { "Manage custom VST3 Folders" };
    juce::Viewport viewport;
    juce::Component rowsHolder;          // Inhalt des Viewports (trägt die Zeilen)
    juce::OwnedArray<Row> rows;
    juce::OwnedArray<EditorWindow> editorWindows;
};
