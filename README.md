# Life Key — by Jay Skilly

Play anything, in any key — Life Key automatically corrects every note to
the scale you pick, so you literally can't hit a wrong note. Works as an
Instrument plugin in Serato Studio (and any other VST3/AU host).

No music theory needed: pick a **Key**, pick a **Scale**, play the keyboard
with your mouse, your laptop keyboard, or a MIDI controller.

## What you need (one-time setup)

1. **Xcode Command Line Tools**:
   ```
   xcode-select --install
   ```
2. **Homebrew** (if you don't have it): https://brew.sh
3. **CMake**:
   ```
   brew install cmake
   ```

## Build it

1. Open Terminal, go to this folder (drag the folder into Terminal after
   typing `cd ` with a space, then hit Enter).

2. Download JUCE (the framework this is built on) — one time:
   ```
   git clone --branch 7.0.12 --depth 1 https://github.com/juce-framework/JUCE.git
   ```

3. Build:
   ```
   cmake -B build -G Xcode
   cmake --build build --config Release
   ```
   Takes a few minutes the first time.

## Install it — the easy way (recommended)

Instead of copying files by hand, build a real installer:

```
./packaging/make_installer.sh
```

This creates **"Life Key Installer.pkg"** in this folder. Double-click it —
you'll get a normal macOS installer window (the same kind any paid plugin
uses), it'll ask for your Mac password, and it installs Life Key
automatically for every DAW on your machine.

## Install it — the manual way (if you'd rather not use the installer)

```
cp -R "build/ScaleLock_artefacts/Release/VST3/Life Key.vst3" ~/Library/Audio/Plug-Ins/VST3/
cp -R "build/ScaleLock_artefacts/Release/AU/Life Key.component" ~/Library/Audio/Plug-Ins/Components/
```

## Load it in Serato Studio

Setup (cog icon) → Plugins tab → click **Scan**. "Life Key" will appear in
your plugin library — drag it onto a new Instrument Deck.

## Using it

- **KEY** — the key you're playing in
- **SCALE** — 26 options: Major/Minor and modes, pentatonics, blues, plus 10
  Hindustani thaats and 5 popular ragas
- **Note correction** (small dropdown, tucked away) — Nearest / Snap Down /
  Snap Up. Nearest is the default and works well for almost everyone.
- **USE MY OWN SOUND** — load one of your own VST3/AU synths so Life Key
  corrects the notes but your synth makes the sound. Click **EDIT** to open
  that synth's own interface and pick a patch. Click the button again
  (now labeled "BUILT-IN SOUND") to switch back.

## Latency

Life Key itself adds 0 samples of delay. For the lowest possible feel:
Serato Studio → Setup → Audio tab → lower the buffer size (try 64 or 128).
If you load your own synth and it has its own internal latency, Life Key
automatically reports that to Serato so timing stays in sync.

## A heads-up about "Use My Own Sound"

Life Key loads your chosen synth inside itself, so Serato only ever sees
one plugin. This works reliably with properly signed, commercially released
synths. If a specific one won't load, that's almost always due to that
plugin's own signing/packaging, not Life Key — try a different synth, or
ask and a virtual-MIDI fallback can be added.

## If the build fails

Usually a JUCE version mismatch — drop `--branch 7.0.12` from the clone
command and use the `develop` branch instead.
