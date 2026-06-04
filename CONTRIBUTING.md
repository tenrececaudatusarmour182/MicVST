# Contributing to MicVST

Thanks for your interest! MicVST is a small, focused Windows tool — bug reports, ideas and
pull requests are all welcome.

## Reporting bugs / requesting features

Open an [issue](https://github.com/philipz794/MicVST/issues). For bugs, please include:

- your Windows version,
- which virtual cable you use (VB-Cable / VoiceMeeter / Virtual Audio Cable / …),
- the plugins in your chain,
- the relevant lines from the log: `%APPDATA%\MicVST\log.txt`.

## Building

Requires **Windows 11 x64** and **Visual Studio 2022** with the *Desktop development with C++*
workload (MSVC + Windows SDK + CMake). JUCE is fetched automatically via CMake `FetchContent`
— nothing else to install.

```powershell
# Configure (fetches JUCE)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# Build app + tests
cmake --build build --config Release --target MicVST MicVSTTests

# Run the unit tests (exit code 0 = ok)
.\build\MicVSTTests_artefacts\Release\MicVSTTests.exe
```

See the README for the full build/run details.

## Pull requests

- Branch off `main` and keep changes focused.
- Match the surrounding code style (JUCE conventions, 4-space indentation).
- Make sure it **builds cleanly with no new warnings** and the **unit tests pass**.
- The GitHub Actions CI builds the app and runs the tests on every push and PR.

## Project layout

```
src/
  Main.cpp                 app lifecycle, tray wiring, silent autostart
  audio/   AudioEngine, PluginChain, GraphConnections, Metering, MicVSTDeviceManager
  state/   Persistence (config.xml), AutostartRegistry
  ui/      MainComponent, DevicePanel, PluginListView, LevelMeterComponent, TrayIcon
tests/     TestMain.cpp (JUCE UnitTest: Metering, GraphConnections, Persistence)
resources/ icon PNGs
```

## License

By contributing, you agree that your contributions are licensed under the project's
**GPL-3.0** license (see [LICENSE](LICENSE)).
