#include "ui/PluginListView.h"
#include "audio/PluginChain.h"

// ============================ TrashButton ============================

void PluginListView::TrashButton::paintButton (juce::Graphics& g, bool highlighted, bool /*down*/)
{
    auto b = getLocalBounds().toFloat();
    const auto col = highlighted ? juce::Colours::orangered : juce::Colours::lightgrey;
    g.setColour (col);

    const float cx = b.getCentreX();
    const float iconH = 15.0f;
    const float top = b.getCentreY() - iconH * 0.5f;
    const float lidY = top + 3.0f;
    const float botY = top + iconH;

    // Deckel-Linie + Griff oben.
    g.drawLine (cx - 7.0f, lidY, cx + 7.0f, lidY, 1.3f);
    g.drawLine (cx - 2.5f, lidY, cx - 2.5f, top + 1.0f, 1.3f);
    g.drawLine (cx - 2.5f, top + 1.0f, cx + 2.5f, top + 1.0f, 1.3f);
    g.drawLine (cx + 2.5f, top + 1.0f, cx + 2.5f, lidY, 1.3f);

    // Korpus (leicht trapezförmig).
    g.drawLine (cx - 5.5f, lidY, cx - 4.5f, botY, 1.3f);
    g.drawLine (cx + 5.5f, lidY, cx + 4.5f, botY, 1.3f);
    g.drawLine (cx - 4.5f, botY, cx + 4.5f, botY, 1.3f);

    // Innere Striche.
    g.drawLine (cx - 2.0f, lidY + 2.5f, cx - 2.0f, botY - 1.5f, 1.0f);
    g.drawLine (cx + 2.0f, lidY + 2.5f, cx + 2.0f, botY - 1.5f, 1.0f);
}

// ============================ Row ============================

PluginListView::Row::Row (PluginListView& ownerIn, int idx) : owner (ownerIn), index (idx)
{
    const auto& es = owner.engine.getChain().entries();
    const bool bypassed = juce::isPositiveAndBelow (index, (int) es.size())
                          && es[(size_t) index].bypassed;

    bypassBtn.setButtonText ("Bypass");
    bypassBtn.setClickingTogglesState (true);
    bypassBtn.setToggleState (bypassed, juce::dontSendNotification);
    bypassBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colours::darkorange);
    bypassBtn.onClick = [this] { owner.toggleBypass (index); };
    addAndMakeVisible (bypassBtn);

    trashBtn.onClick = [this] { owner.requestRemove (index); };
    addAndMakeVisible (trashBtn);
}

bool PluginListView::Row::overHandle (juce::Point<int> p) const { return p.x < 24; }

void PluginListView::Row::resized()
{
    auto r = getLocalBounds().reduced (2);
    trashBtn.setBounds (r.removeFromRight (28));
    r.removeFromRight (4);
    bypassBtn.setBounds (r.removeFromRight (80));
}

void PluginListView::Row::paint (juce::Graphics& g)
{
    auto b = getLocalBounds();
    const auto& es = owner.engine.getChain().entries();
    if (! juce::isPositiveAndBelow (index, (int) es.size())) return;
    const auto& e = es[(size_t) index];

    g.setColour (dragging ? juce::Colours::darkblue
                          : (index % 2 ? juce::Colour (0xff2a2a2a) : juce::Colour (0xff242424)));
    g.fillRect (b);

    // Drag-Handle: zwei Spalten à drei Punkte.
    g.setColour (juce::Colours::grey);
    for (int cxn = 0; cxn < 2; ++cxn)
        for (int cyn = 0; cyn < 3; ++cyn)
            g.fillEllipse ((float) (8 + cxn * 6), (float) (b.getCentreY() - 6 + cyn * 6), 2.6f, 2.6f);

    // Latenz-Spalte links vom Bypass: gemeldete Plugin-Latenz in ms (Lookahead etc.).
    constexpr int latW = 72;
    const int latRight = bypassBtn.getX() - 8;
    const int latLeft  = latRight - latW;

    juce::String latText;
    if (auto* node = owner.engine.getGraph().getNodeForId (e.node))
        if (auto* proc = node->getProcessor())
        {
            auto* dev = owner.engine.getDeviceManager().getCurrentAudioDevice();
            const double sr = dev != nullptr ? dev->getCurrentSampleRate() : 48000.0;
            const double ms = sr > 0.0 ? (double) proc->getLatencySamples() / sr * 1000.0 : 0.0;
            latText = juce::String (ms, 1) + " ms";
        }

    // Name (auf den Platz links der Latenz-Spalte begrenzt).
    const auto name = e.fileOrId == PluginChain::monoToStereoId ? juce::String ("Mono -> Stereo")
                    : e.fileOrId == PluginChain::stereoToMonoId ? juce::String ("Stereo -> Mono")
                    : juce::File (e.fileOrId).getFileNameWithoutExtension();
    g.setColour (e.bypassed ? juce::Colours::grey : juce::Colours::white);
    g.drawText (name, 28, 0, latLeft - 6 - 28, b.getHeight(), juce::Justification::centredLeft);

    g.setColour (juce::Colours::grey);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText (latText, latLeft, 0, latW, b.getHeight(), juce::Justification::centredRight);
}

void PluginListView::Row::mouseDown (const juce::MouseEvent& e)
{
    if (overHandle (e.getPosition()))
    {
        dragging = true;
        grabOffsetY = e.getPosition().y;
        toFront (false);
    }
}

void PluginListView::Row::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const int n = owner.rows.size();
    const int maxY = juce::jmax (0, (n - 1) * rowH);
    const int newY = juce::jlimit (0, maxY, e.getEventRelativeTo (getParentComponent()).y - grabOffsetY);
    setTopLeftPosition (getX(), newY);
}

void PluginListView::Row::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    dragging = false;
    const int n = owner.rows.size();
    const int target = juce::jlimit (0, n - 1, (getY() + rowH / 2) / rowH);
    owner.requestMove (index, target);   // rebuildRows() snappt alles zurück an Position
}

void PluginListView::Row::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! overHandle (e.getPosition()))
        owner.openEditor (index);
}

// ============================ PluginListView ============================

PluginListView::PluginListView (AudioEngine& e) : engine (e)
{
    addAndMakeVisible (addBtn);
    addBtn.onClick = [this] { showAddMenu(); };

    addAndMakeVisible (manageFoldersBtn);
    manageFoldersBtn.onClick = [this] { showFolderMenu(); };

    viewport.setViewedComponent (&rowsHolder, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    rebuildRows();
    startTimer (500);   // Plugin-Latenz live halten (z. B. wenn ein Plugin Lookahead an-/abschaltet)
}

void PluginListView::timerCallback()
{
    for (auto* r : rows)
        r->repaint();   // nur neu zeichnen; die Latenz wird in Row::paint frisch gelesen
}

void PluginListView::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (30);
    constexpr int btnH = 26;   // einheitliche Button-Höhe (wie Row-/DevicePanel-Buttons)
    addBtn.setBounds (top.removeFromLeft (110).withSizeKeepingCentre (106, btnH));
    manageFoldersBtn.setBounds (top.removeFromRight (200).withSizeKeepingCentre (196, btnH));   // rechtsbündig
    r.removeFromTop (4);
    viewport.setBounds (r);

    const int vh = viewport.getHeight();
    const int contentH = rows.size() * rowH;
    // Scrollbar-Breite reservieren, wenn die Kette überläuft -> der Mülleimer am
    // rechten Rand verschwindet nie hinter dem Scrollbalken.
    const int w = viewport.getWidth() - (contentH > vh ? viewport.getScrollBarThickness() : 0);
    rowsHolder.setSize (w, juce::jmax (vh, contentH));
    for (int i = 0; i < rows.size(); ++i)
        rows[i]->setBounds (0, i * rowH, w, rowH);
}

void PluginListView::rebuildRows()
{
    rows.clear();
    const int n = (int) engine.getChain().entries().size();
    for (int i = 0; i < n; ++i)
    {
        auto* row = new Row (*this, i);
        rowsHolder.addAndMakeVisible (row);
        rows.add (row);
    }
    resized();
}

void PluginListView::commitChange()
{
    engine.rebuildGraph();
    saveState (engine.captureState());
    rebuildRows();
}

void PluginListView::requestRemove (int index)
{
    const auto& es = engine.getChain().entries();
    if (! juce::isPositiveAndBelow (index, (int) es.size())) return;
    const auto& e = es[(size_t) index];
    const auto name = e.isBuiltIn() ? juce::String ("Mono -> Stereo")
                                    : juce::File (e.fileOrId).getFileNameWithoutExtension();

    // Sicherheitsabfrage. Der Dialog ist asynchron -> die auslösende Row bleibt am Leben,
    // bis OK gedrückt wird (das eigentliche Entfernen passiert erst im Callback).
    juce::NativeMessageBox::showOkCancelBox (
        juce::MessageBoxIconType::QuestionIcon,
        "Remove plugin",
        "Remove \"" + name + "\" from the chain?",
        nullptr,
        juce::ModalCallbackFunction::create ([this, index] (int result)
        {
            if (result == 0) return;   // cancel
            if (! juce::isPositiveAndBelow (index, (int) engine.getChain().entries().size())) return;
            editorWindows.clear();     // offene Editoren schließen (sonst Dangling -> Crash)
            engine.getChain().removePlugin (index);
            commitChange();
        }));
}

void PluginListView::requestMove (int from, int to)
{
    juce::MessageManager::callAsync ([this, from, to]
    {
        const int n = (int) engine.getChain().entries().size();
        if (from == to || ! juce::isPositiveAndBelow (from, n)) { rebuildRows(); return; }
        engine.getChain().movePlugin (from, juce::jlimit (0, n - 1, to));
        commitChange();
    });
}

void PluginListView::toggleBypass (int index)
{
    juce::MessageManager::callAsync ([this, index]
    {
        const auto& es = engine.getChain().entries();
        if (! juce::isPositiveAndBelow (index, (int) es.size())) return;
        engine.getChain().setBypass (index, ! es[(size_t) index].bypassed);
        commitChange();
    });
}

void PluginListView::showAddMenu()
{
    auto& known = engine.getKnownPlugins();
    constexpr int monoToStereoItemId = 100000;   // außerhalb des Plugin-Index-Bereichs
    constexpr int stereoToMonoItemId = 100001;

    juce::PopupMenu m;
    m.addItem (monoToStereoItemId, "Mono -> Stereo (built-in)");
    m.addItem (stereoToMonoItemId, "Stereo -> Mono (built-in)");
    m.addSeparator();
    auto types = known.getTypes();
    for (int i = 0; i < types.size(); ++i)
        m.addItem (i + 1, types[i].name);

    m.showMenuAsync (juce::PopupMenu::Options(), [this, types, monoToStereoItemId, stereoToMonoItemId] (int res)
    {
        if (res <= 0) return;

        if (res == monoToStereoItemId)
        {
            engine.getChain().addMonoToStereo();
            commitChange();
            return;
        }

        if (res == stereoToMonoItemId)
        {
            engine.getChain().addStereoToMono();
            commitChange();
            return;
        }

        auto desc = types[res - 1];
        double sr = engine.getDeviceManager().getCurrentAudioDevice() != nullptr
                  ? engine.getDeviceManager().getCurrentAudioDevice()->getCurrentSampleRate() : 48000.0;
        juce::String err;
        if (engine.getChain().addPlugin (engine.getFormatManager(), desc, sr, 128, err))
            commitChange();
        else
            juce::Logger::writeToLog ("Plugin-Load: " + err);
    });
}

void PluginListView::showFolderMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Add folder...");
    m.addSeparator();

    const auto& folders = engine.getPluginFolders();
    if (folders.isEmpty())
    {
        m.addItem (2, "(no custom folders)", false /*enabled*/, false);
    }
    else
    {
        // Pro Ordner ein Untermenü mit "Remove". IDs ab 1000 = Index in der Ordnerliste.
        for (int i = 0; i < folders.size(); ++i)
        {
            juce::PopupMenu sub;
            sub.addItem (1000 + i, "Remove");
            m.addSubMenu (folders[i], sub);
        }
    }

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&manageFoldersBtn),
        [this] (int res)
        {
            if (res == 1) { chooseFolder(); return; }
            if (res >= 1000)
            {
                const auto& f = engine.getPluginFolders();
                const int idx = res - 1000;
                if (juce::isPositiveAndBelow (idx, f.size()))
                {
                    engine.removePluginFolder (f[idx]);
                    saveState (engine.captureState());
                }
            }
        });
}

void PluginListView::chooseFolder()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Choose a VST3 folder");
    chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser] (const juce::FileChooser& fc)
        {
            const auto dir = fc.getResult();
            if (! dir.isDirectory()) return;
            engine.addPluginFolder (dir.getFullPathName());   // hinzufügen + rescan
            saveState (engine.captureState());                // Ordner persistieren
        });
}

void PluginListView::openEditor (int row)
{
    const auto& es = engine.getChain().entries();
    if (! juce::isPositiveAndBelow (row, (int) es.size())) return;
    auto* node = engine.getGraph().getNodeForId (es[(size_t) row].node);
    if (node == nullptr) return;
    auto* proc = node->getProcessor();
    if (proc == nullptr || ! proc->hasEditor()) return;

    auto* w = new EditorWindow (proc->getName(),
                                [this] (EditorWindow* self) { editorWindows.removeObject (self); });
    w->setUsingNativeTitleBar (true);
    w->setContentOwned (proc->createEditorAndMakeActive(), true);
    w->centreWithSize (w->getWidth(), w->getHeight());
    w->setVisible (true);
    editorWindows.add (w);
}
