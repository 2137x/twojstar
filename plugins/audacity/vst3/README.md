# Audacity VST3

Cross-platform VST3 **audio effects** aimed first at Audacity and usable in other compatible hosts.

## Auto Declip 0.1

The first released effect is intentionally conservative. Its repair pipeline handles short full-scale clipping plateaus, isolated single-sample impulse clicks, and narrow mains hum without pretending that severely missing audio can be recovered by a magic button.

Current detector/repair rules:

- clipping threshold: `|sample| >= 0.995`,
- repair only consecutive runs of **2–32 samples**,
- the declipping stage preserves isolated full-scale samples unless the de-click stage sees strong smooth-context evidence for a one-sample impulse,
- de-click requires two smooth context samples on each side and leaves multi-sample transients alone,
- longer clipping is preserved for a future stronger restoration stage,
- de-hum uses narrow Q=35 notches at 50/60 Hz plus 100/120 and 150/180 Hz harmonics,
- de-hum adds no algorithmic latency and resets its filter state after a non-finite sample,
- audio outside the narrow repair bands remains effectively transparent; broadband `Denoise` is exported as an explicit VST3 on/off parameter and defaults to **Off**,
- repaired runs are reconstructed between clean edges with a bounded peak-shaped interpolation,
- output is capped just below full scale,
- fixed **66-sample pipeline latency** (64 declip + 2 de-click), reported to the VST3 host,
- mono and stereo, 32-bit and 64-bit floating-point processing,
- fixed memory only in the audio path; no allocations, files, network or model loading.

### De-noise core prototype

`DeNoiseDsp` is a separate conservative broadband downward expander for low-level stationary noise. It uses fixed-memory envelope/gain smoothing, has no algorithmic latency, never hard-mutes the floor, and resets safely after non-finite input. It is wired behind the explicit `Denoise` VST3 parameter, defaults to **Off**, and the selected state is stored with the host project/preset. Quiet ambience and reverb tails are therefore not altered unless the user enables broadband repair.

## Smart Transition 0.1 prototype

Smart Transition is the first implementation from the [`../smart-edit/`](../smart-edit/) track. It targets the little click/thump/level jump left after a cut or join.

The prototype is deliberately deterministic DSP first:

- 100 ms bounded lookahead, clamped to 1024–8192 samples,
- mono/stereo shared seam detection without signed cross-channel cancellation,
- block-partition-independent candidate ordering and score quantization,
- short local DC and level matching,
- a bounded S-curve/Hermite bridge around the accepted seam,
- one high-confidence seam per VST3 processing run,
- exact same-format determinism tests across arbitrary input chunking,
- no allocation, network, model loading or background helper in the audio path.

The effect currently uses conservative fixed defaults while the host contract is being validated. The planned `Mode`, `Max transition`, `Strength` and `Repair` host parameters are still required before calling 0.1 release-ready.

### Why it is not in Latest yet

CI builds and packages `TravnySmartTransition.vst3` for Windows and Linux as an **experimental workflow artifact**, but the repository-wide rolling **Latest** release intentionally does not publish it yet.

Generic VST3 `ProcessData` does not provide a portable end-of-selection marker. The host-independent DSP core can finalize a late seam and shrink context for a short selection when its caller invokes `drainFrame()`, but the current VST3 adapter does not capture such a boundary and only calls `processFrame()`. End-of-selection behavior therefore remains a release gate even though the validated Audacity fixture below produces the expected output.

Windows host validation on **Audacity 3.7.9** now passes discovery, Preview, Apply, Undo, cancellation, and host responsiveness. An 80-frame stereo fixture at 48 kHz with its seam at frame 40 was processed end-to-end: Apply preserved all 80 output frames and repaired the seam, validating Audacity's observed output for this specific case. Preview → Stop → Cancel left no Smart Transition undo entry, and the next Apply produced byte-identical PCM to the earlier Apply, confirming state reset and deterministic reprocessing for the same fixture.

Before promotion, the adapter or host test harness must explicitly exercise end-of-selection handling for late seams and short selections, different real host block sizes must produce the same plan/output contract, and the planned `Mode`, `Max transition`, `Strength`, and `Repair` parameters remain required for 0.1. Until those gates pass, the workflow ZIP stays a development artifact rather than a supported release.

## Released packages

Ready-to-copy Auto Declip packages are published in the repository-wide GitHub **Latest** release:

- [Windows x64](https://github.com/travnie/twojstar/releases/latest/download/audacity-auto-declip-windows.zip)
- [Linux x64](https://github.com/travnie/twojstar/releases/latest/download/audacity-auto-declip-linux.zip)

Each archive contains the `TravnyAutoDeclip.vst3` bundle plus a tiny `INSTALL.txt`. On Windows x64, copy the `.vst3` bundle to `C:\Program Files\Common Files\VST3`; on Linux, use your standard VST3 plug-in directory. Then rescan effects in Audacity. The repository does not install a background helper and the plug-ins perform no runtime downloads.

## Layout

```text
vst3/
├── CMakeLists.txt
├── src/autodeclip/
│   ├── AutoDeclipDsp.*
│   ├── AutoDeclipProcessor.*
│   └── ... VST3 adapter files
├── src/smarttransition/
│   ├── SmartTransitionDsp.*
│   ├── SmartTransitionProcessor.*
│   └── ... VST3 adapter files
└── tests/
    ├── AutoDeclipDspTests.cpp
    └── SmartTransitionDspTests.cpp
```

Both DSP cores are independent of the VST3 SDK so they can be unit-tested on Windows and Linux without the host adapter.

## Build the DSP tests only

No third-party SDK is needed:

```bash
cmake -S plugins/audacity/vst3 -B plugins/audacity/vst3/build -DBUILD_TESTING=ON
cmake --build plugins/audacity/vst3/build --config Release
ctest --test-dir plugins/audacity/vst3/build -C Release --output-on-failure
```

## Build the VST3 effects

Use a separately reviewed Steinberg VST3 SDK 3.8+ checkout. The repository does **not** download or execute an SDK at configure time.

```bash
cmake -S plugins/audacity/vst3 -B plugins/audacity/vst3/build-sdk \
  -DVST3_SDK_ROOT=/path/to/vst3sdk \
  -DBUILD_TESTING=ON \
  -DSMTG_CREATE_PLUGIN_LINK=OFF
cmake --build plugins/audacity/vst3/build-sdk --config Release \
  --target TravnyAutoDeclip TravnySmartTransition
```

CI pins Steinberg VST3 SDK **3.8.0** by commit and checks its submodules recursively on both Windows and Linux. VSTGUI and SDK examples are disabled because neither effect needs them.

## Product rules

- **Format:** VST3 audio effect.
- **Language:** C++20.
- **Build:** CMake is the source of truth.
- **Targets:** Windows x64 and Linux x64 first; macOS can follow without redesigning DSP.
- **Runtime:** local/offline, deterministic, no telemetry/login/background service/runtime download.
- **UI:** host parameters first; custom GUI only when an effect genuinely needs one.
- **Performance:** bounded memory and real-time-safe processing where applicable.

## Before calling Auto Declip stable

The initial implementation still needs real recordings and generated clipping fixtures beyond the unit tests. Before a stable release, validate:

1. Steinberg validator/test-host behavior,
2. Audacity scan/load and latency compensation,
3. mono/stereo and 32/64-bit paths,
4. block-boundary clipping runs,
5. false-positive rate on hard-limited but intentionally undamaged masters,
6. quality against Audacity Clip Fix and other deterministic baselines.

Neural declipping is a later stage, not a branding sticker glued over an interpolation function.
