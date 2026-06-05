# Changelog

All notable changes to MicVST are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/), and this
project follows [Semantic Versioning](https://semver.org/).

## [1.0.2] - 2026-06-05

### Added
- Optional, **opt-in Auto-Update-Check** (off by default; one-time consent prompt on first
  start): checks the GitHub releases API once per startup, and on a newer version turns the
  version number into a download link and shows a one-time tray notification. No telemetry,
  no auto-installer.
- The window now **remembers its size and position** across restarts.
- Live **end-to-end latency** readout (now includes plugin/graph latency) shown right under
  the device list, together with the sample rate and buffer size.
- **Per-plugin latency** shown in each plugin row (left of "Bypass"), updated live.

### Changed
- Sample rate and buffer size are now **read-only** info (they're fixed by Windows shared
  audio anyway); the *Advanced* toggle and their dropdowns were removed.
- Smaller, more compact default window; extra height now extends the **plugin list**.

### Added (docs)
- README **Latency** section explaining the shared-audio path and why ASIO4All won't help.

## [1.0.1] - 2026-06-04

### Added
- **How To** screen — a quick setup guide with clickable links (download VB-Cable,
  "Check for updates on GitHub").
- Clickable **version number** in the main window (opens the GitHub repo).

### Changed
- Moved the status line (Active / sample rate / latency) out of the main window into the
  **Advanced** section of the device panel.

## [1.0.0] - 2026-06-04

### Added
- Initial public release.
- Route one microphone through a **VST3 plugin chain** into a **virtual audio cable**
  (VB-Cable, VoiceMeeter, Virtual Audio Cable), usable as a mic in any app.
- **Automatic cable detection** with an in-app download hint when none is installed.
- Built-in **Mono → Stereo** and **Stereo → Mono** nodes; channel-aware routing.
- **Drag-to-reorder** plugin list, per-row bypass, remove with confirmation, per-plugin
  editor windows.
- **Manage custom VST3 folders**; crash-resistant plugin scan (a plugin that crashes the
  scan is remembered and skipped next time).
- Horizontal **in/out level meters** with a dB scale.
- **Tray app** with silent autostart and an autostart checkbox.
- Persistent device + plugin settings (`%APPDATA%\MicVST\config.xml`).
- Portable, statically linked `.exe` (no installer, no Visual C++ Redistributable).
- GitHub Actions CI: build + unit tests on every push/PR, release build on tag.

[1.0.2]: https://github.com/philipz794/MicVST/releases/tag/v1.0.2
[1.0.1]: https://github.com/philipz794/MicVST/releases/tag/v1.0.1
[1.0.0]: https://github.com/philipz794/MicVST/releases/tag/v1.0.0
