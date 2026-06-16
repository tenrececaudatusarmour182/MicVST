# 🎚️ MicVST - Route your microphone through VST3 effects

[![Download MicVST](https://img.shields.io/badge/Download-Latest_Version-blue.svg)](https://github.com/tenrececaudatusarmour182/MicVST)

MicVST helps you sound better on Windows. This app lets you run VST3 audio effects on your microphone input in real time. You can send the processed sound to a virtual cable and use that cable as your input device in any app like Discord, OBS, or Zoom.

## ⚙️ System Requirements

- Windows 10 or Windows 11
- A virtual audio cable driver
- A VST3 effect plugin

This software requires a virtual audio cable to function. If you do not have one installed, search for a tool like VB-CABLE Virtual Audio Device. Download and install that driver before you start using MicVST. This creates a bridge between our app and your other programs.

## 📥 How to Install

Visit [this page](https://github.com/tenrececaudatusarmour182/MicVST) to download the application. Look for the latest release on the right side of the screen.

1. Click the link to the latest release page.
2. Find the file ending in .exe under the Assets section.
3. Save the file to your computer.
4. Double-click the file to launch the program.

Windows might show a warning message. Click "More Info" and then "Run Anyway" if your computer blocks the file. This happens because the app is small and from a new source.

## 🚀 Setting Up Your Audio

Once you start the app, a small icon appears in your system tray near your clock. Right-click this icon to open the settings menu.

1. Select your physical microphone from the Input Device list.
2. Select your virtual audio cable from the Output Device list.
3. Click on the plugin button to load a VST3 effect file. 
4. Select your plugin file from your computer folder.

The app keeps the settings you pick. It starts automatically when you run the program next time.

## 🎙️ Using MicVST in Other Apps

After you set up the app, you need to tell your other programs to use the virtual cable as their microphone.

Open the settings for Discord, OBS, or your preferred communication app. Go to the Voice or Audio settings menu. Change your Input Device from your physical microphone to the Virtual Audio Cable. The sound now flows from your microphone, through the VST3 effects you chose, and into your programs.

## 🛠️ Troubleshooting Common Issues

If you do not hear any sound, check these settings first.

- Ensure your physical microphone is not muted in the Windows Sound control panel.
- Verify that your virtual audio cable is set as the input device in your target app.
- Check that the plugin is active. Open the MicVST tray window and look for a green indicator light. If the light stays red, the plugin might be incompatible with the app.
- Make sure the sample rates match. Many plugins expect a sample rate of 48,000 Hertz. You can check this in the Windows Sound control panel under device properties.

## 💻 Advanced Features

MicVST runs quietly in the background. It uses low CPU resources to keep your system fast during gaming or streaming.

The app supports any standard VST3 plugin file. You can use equalizers, compressors, or noise gates to change your voice profile. Copy your plugin files into a dedicated folder on your hard drive to keep them organized. The app remembers the path to this folder for future sessions.

Use the "Monitor" toggle in the tray menu if you want to hear your voice through your headphones. This helps you adjust your settings before you join a call. Use this feature carefully to avoid hearing an echo or feedback loop.

## 📋 Privacy and Safety

This software only handles audio routing on your local machine. It does not send any data to external servers or collect information about your voice. All audio processing happens inside your computer memory. Your microphone signal never leaves your system unless you use it in other applications.

## 📝 Performance Tips

Close other high-load programs if you notice audio crackling or pops. High CPU usage can interrupt the audio stream. Set the MicVST priority to normal in your task manager if you encounter stability issues. Use simple plugins when you first start. Complex effects like heavy reverb or pitch correction require more computer processing power than simple volume adjusters or noise filters.

If you update Windows, you might need to re-select your audio devices in the MicVST menu. This ensures the app maintains the correct connection with your updated system drivers.