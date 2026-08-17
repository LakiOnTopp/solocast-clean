# SoloCast Clean â€” OBS audio filter (V0.2)

A lightweight native OBS audio filter tuned for the HyperX SoloCast, with natural voice deepening and clarity controls.

## What it does

- Removes low desk thumps / rumble with a high-pass stage
- **Deep Voice**: adds natural low-end/body without the robotic sound of a pitch shifter
- **Clarity**: adds speech presence so the voice stays crisp
- Reduces muddiness around the low mids
- Smooth downward expander for keyboard / mouse / room noise while you are not speaking
- De-esser for harsh S sounds
- Compressor for a denser, more consistent voice
- Soft limiter to avoid clipping

## Recommended SoloCast preset

- Noise / desk threshold: **-42 dB**
- Background reduction: **-24 dB**
- Deep Voice: **2.0**
- Clarity: **2.5 dB**
- Compression: **3.0**
- De-esser: **4.0**
- Output: **0 dB**

### Deep Voice

Start around **2.0**. Try **2.5â€“3.5** if you want a noticeably deeper, warmer voice. Above roughly **4.0**, some voices may become too bass-heavy.

This control intentionally changes the tone/body of the voice instead of shifting the actual pitch. That keeps speech much more natural and avoids the typical artificial â€œvoice changerâ€ artifacts.

### Clarity

Start at **2.5 dB**. If the voice sounds dull, increase it. If consonants become too sharp, lower it or increase De-esser slightly.

## Keyboard / desk noise

If keyboard sounds still pass while you are silent, raise **Noise / desk threshold** gradually from -42 dB toward -38 dB. If the beginning/end of words starts disappearing, lower it again.

## Important limitation

This version uses classic real-time DSP, not neural source separation. It handles desk vibrations, low rumble and background noise between phrases well, but cannot perfectly remove a keyboard click occurring at exactly the same time as speech.

## Build on Windows

Requirements:

- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.24+
- OBS Studio source tree matching or close to your installed OBS version
- Matching 64-bit libobs import library (`obs.lib` / `libobs.lib`)

Example Developer PowerShell:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DOBS_SOURCE_DIR="C:/src/obs-studio" `
  -DLIBOBS_LIBRARY="C:/path/to/obs.lib"
cmake --build build --config Release
```

The output DLL is `solocast-clean.dll`.

## Installing in OBS

A common manual layout is:

```text
obs-studio/
  obs-plugins/64bit/solocast-clean.dll
```

Restart OBS, then open your HyperX SoloCast source â†’ **Filters** â†’ **+** â†’ **SoloCast Clean**.

## Mic placement

For best results, place the SoloCast roughly 10â€“15 cm from your mouth and avoid leaving its stock stand directly coupled to the desk if possible. A boom arm / shock mount will reduce desk impacts more effectively than software alone.

