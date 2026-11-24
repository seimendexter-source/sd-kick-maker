# SD KICK MAKER

Kick-Baukasten für Hard Techno / Industrial von **Seimen Dexter** (DEXIT Records).
VST3 + AU für macOS (Apple Silicon und Intel), gebaut mit JUCE.

## Funktionen
- Kick-Synth, automatisch auf die **Tonart** des Tracks gestimmt (Oktave 1, Tune ±12 Halbtöne)
- Regler: **Punch, Body, Länge** (Anteil eines Beats, folgt dem Ableton-Tempo), **Drive** (4x Oversampling, Fold ab 55 %), **Rumble** (dunkles Hall-Netz, gefiltert, verzerrt, geduckt), **Output**
- **Sample-Modus**: eigene Kick reinziehen → Grundton wird erkannt und auf die Tonart umgestimmt
- Presets, A/B-Vergleich, **Export WAV** (One-Shot, 1 Beat, 24 Bit) nach `~/Music/SD KICK MAKER`

## Build
Jeder Push auf `main` baut automatisch über GitHub Actions. Das fertige Plugin liegt danach unter **Releases**.

Lokal: `cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" && cmake --build build --config Release`
