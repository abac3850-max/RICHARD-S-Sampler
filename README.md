# RA Sampler — JUCE VST3 source project

Development snapshot: **v0.3.0**. RA Sampler is a Windows 64-bit JUCE/C++ sampler instrument project.

## Implemented in this snapshot

- VST3 and Standalone CMake targets (JUCE 8.0.10).
- 8 visible sample slots and a 64-voice polyphonic engine.
- Resizable interface.
- Ten colour-only skins applied to the interface: Dark Green, Midnight Violet, Ocean Teal, Sunset Orange, Ruby Red, Arctic Silver, Gold Edition, Electric Lime, Sapphire, Rose Quartz.
- Selected-slot waveform is drawn from the actual loaded audio buffer (not a placeholder).
- Drag handles for sample start/end, loop start/end, fade-in endpoint and fade-out endpoint.
- Fade-in and fade-out are applied in the playback voice.
- Rotary-style controls use a custom green-accent LookAndFeel; horizontal fade controls are provided.
- Factory kit selector loads the five included `.rapreset` kits into the eight slots in one action.
- Original factory library: 64 generated WAV one-shots (8 each: Kick, Snare, Clap, Closed Hat, Open Hat, Rim, Tom, Percussion).
- Slot editing ranges and fade settings are serialized in plugin state. Sample audio files themselves are not embedded yet.

## Waveform editing

Select a loaded slot to display its audio waveform. Drag the two green outer handles to set sample start/end. Drag the orange inner handles to set loop start/end. The fade-in and fade-out handles control the fade region; the matching horizontal controls are also available below the waveform. The shaded waveform regions show the fade areas.

## Factory kits

The factory kit selector contains RA Core Drum Kit, RA Hip-Hop Kit, RA Electronic Kit, RA Afro Percussion Kit, and RA Minimal Kit. Kit manifests reference the included FactoryLibrary WAV files. CMake currently embeds the source-tree FactoryPresets path for development builds; a later installer/resource-packaging pass should move assets to a portable installed location.

## Audio format support

The file chooser offers WAV, AIFF, FLAC, OGG and MP3. Actual decoding depends on the JUCE build and enabled platform codecs. AAC/M4A/WMA are not promised in this snapshot and need additional decoder support.

## Build on Windows 10/11

Requirements: Visual Studio 2022 with **Desktop development with C++** and Windows SDK, CMake 3.22 or newer, Git, and internet access on first configure/build (CMake fetches JUCE 8.0.10).

In an x64 Native Tools Command Prompt:

```bat
cmake -S . -B build -A x64
cmake --build build --config Release
```

The VST3 should be produced under `build\RASampler_artefacts\Release\VST3\RA Sampler.vst3` (exact path can vary).

## Verification status and known limitations

This is an updated source snapshot, **not a compiled or DAW-tested binary**. Build verification could not be completed in the current environment because it could not reach GitHub to fetch JUCE. The source still needs a Windows build and Studio One test. The sample editing parameters and fades are implemented in the voice renderer; advanced editing polish, undo/redo, non-destructive waveform zoom, tempo-sync, slicing, sample embedding/relinking, and full preset management remain future work. The pan and pitch controls are currently visual controls and are not yet wired into the playback engine. Review JUCE licensing before commercial distribution.
