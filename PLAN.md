# Seiche Implementation Plan

> Roadmap for filling the gaps in the current Qt-based acoustics application while keeping the existing editor workflow intact.

## Phase 0 — Tooling and Repo Hygiene

### Milestone: reproducible builds and automated verification

> The app already runs. This phase makes it easier to trust changes before extending the acoustics feature set.

- [x] Add CI for Linux, Windows, and macOS builds
- [x] Add CMake presets for debug, release, and test configurations
- [x] Add a formatting/linting pass for C++ sources and headers
- [x] Add a test runner script that builds `tests/` and executes all Qt tests headlessly
- [x] Add build-status badges and a short contributor note to `README.md`

## Phase 1 — Core Data Model and File Format

### Milestone: deterministic project files and clearer model boundaries

> The current `.room` workflow stores the important data, but it still needs a more explicit contract and stronger validation.

- [x] Split project serialization concerns from UI concerns in `src/core/ProjectFile.*`
- [x] Add explicit schema and validation rules for meshes, materials, sources, listeners, and sample rate
- [x] Make project save/load round-trips deterministic so tests can compare file contents
- [x] Add tests for malformed project files and missing resource references
- [x] Document the `.room` format and coordinate conventions in `INSTRUCTIONS.md`

## Phase 2 — Headless Simulation and Batch Export

### Milestone: render scenes without the GUI

> The app is GUI-first today. A headless path makes it easier to automate validation, benchmark changes, and compare outputs across solvers.

- [x] Add a command-line render entry point that loads a project and writes RIR/WAV output
- [x] Separate simulation parameter handling from dialog widgets in `src/gui/dialogs/RenderOptionsDialog.*`
- [x] Add a batch renderer that can process all listeners in a project without user interaction
- [x] Add export helpers for WAV, CSV, and a compact metrics summary
- [x] Add tests that render a small fixture project and compare output metadata

## Phase 3 — True Binaural Rendering

### Milestone: listener output that models head orientation and HRTFs

> The current stereo output path is useful, but it is still not the same as a real binaural pipeline.

> SOFA is the primary target format for this phase. The implementation should introduce a format-agnostic HRTF layer first, then load SOFA datasets through `libmysofa`. DAFF/OpenDAFF support is a follow-on integration point and should not block the first deliverable.

#### Phase 3.1 — HRTF model and data loading

- [ ] Define a format-agnostic HRTF dataset interface for lookup, metadata, and filter length
- [ ] Add a SOFA-backed dataset loader using `libmysofa`
- [ ] Normalize the in-memory representation so the renderer can query any dataset by direction
- [ ] Add validation for missing metadata, unsupported sample rates, and invalid filter lengths
- [ ] Add a synthetic HRTF fixture that exercises the lookup path without external files

#### Phase 3.2 — Binaural lookup and listener orientation

- [ ] Convert listener orientation into a head-centric listener frame
- [ ] Add head-orientation-aware direction lookup before HRTF convolution
- [ ] Add nearest-neighbor or interpolated HRTF selection for azimuth/elevation queries
- [ ] Define fallback behavior for missing or out-of-range directions
- [ ] Add tests that confirm the same source produces different left/right responses as the listener turns

#### Phase 3.3 — Audio rendering modes and export

- [ ] Add binaural export modes for mono, stereo, and per-listener output
- [ ] Keep the existing mono/stereo room output path available for non-binaural comparison workflows
- [ ] Make HRTF selection part of the render job parameters and output metadata
- [ ] Add per-listener file naming and directory layout rules for binaural renders
- [ ] Add tests that verify export shape, filenames, and sample-rate handling

#### Phase 3.4 — UI and project persistence

- [ ] Add a UI path for selecting an HRTF dataset per listener
- [ ] Persist the selected HRTF dataset in project data and restore it on load
- [ ] Show a clear default state when no external HRTF dataset is selected
- [ ] Add a minimal inspection view for the selected dataset metadata
- [ ] Add tests for dialog behavior and project round-tripping

#### Phase 3.5 — Future-format seam

- [ ] Keep the HRTF interface isolated so DAFF/OpenDAFF support can be added without changing listener rendering
- [ ] Add a loader registry or adapter layer that can accept additional HRTF formats later
- [ ] Document the expected extension points for a future OpenDAFF implementation
- [ ] Add one integration test placeholder that can be reused when DAFF support lands

## Phase 4 — Source Directivity

### Milestone: directional emitters instead of only omni sources

> This closes a major gap versus other acoustics toolchains and allows the app to model loudspeakers more realistically.

- [ ] Add a source directivity interface with omni and cardioid defaults
- [ ] Add a GLL-backed directivity loader for loudspeaker balloons
- [ ] Apply source directivity in both the image-source and ray-tracing paths
- [ ] Add a small visual directivity diagnostic in the UI
- [ ] Add tests for rotated sources and rear-facing suppression

## Phase 5 — Solver Architecture Cleanup

### Milestone: a clearer separation between the editor and the propagation engines

> The solver code is already capable, but it is embedded deeply in the application flow. Pulling it behind narrower interfaces will reduce the cost of new methods.

- [ ] Define a solver interface that covers ISM, ray tracing, and DG paths
- [ ] Move shared acoustic computations into reusable core utilities
- [ ] Add a common result format for impulse responses and acoustic metrics
- [ ] Reduce duplication between `AcousticSimulator`, `SimulationWorker`, and the GUI render actions
- [ ] Add tests that run each solver through the same fixture project

## Phase 6 — GPU Implementation

### Milestone: replace the current GPU placeholders with real acceleration

> The repository already has GPU-oriented types and fallback paths. The missing piece is actual GPU work that outperforms the CPU path on real scenes.

- [ ] Implement the GPU convolver instead of falling back to CPU FFTs
- [ ] Implement the GPU ray tracer path instead of the warning-only stub
- [ ] Add feature detection and graceful fallback when GPU support is unavailable
- [ ] Benchmark CPU vs. GPU for representative room sizes and ray counts
- [ ] Add tests that verify GPU and CPU results stay within an acceptable tolerance

## Phase 7 — Coverage Foundation and High-Risk Gaps

### Milestone: measurable repo-wide coverage above 80%

> The existing suite exercises the main paths, but the coverage target is not yet enforced or evenly distributed. This phase makes coverage measurable first, then pushes the weak spots over the threshold with focused tests instead of broad refactors.

> Current source-only coverage from `gcovr` on `src/` sources, excluding tests, generated MOC/compiler-id files, and system headers: 23.3% line coverage, 26.3% function coverage, and 13.5% branch coverage.

- [x] Add a repeatable Linux coverage job using `gcovr` that reports `src/` line coverage, excludes tests and generated files, and fails the build when coverage drops below 80%
- [x] Add unit tests for `src/core/MaterialLoader.*`, `src/core/ProjectFile.*`, and `src/utils/ResourcePath.*` to cover malformed inputs, missing files, and canonical round-trips
- [x] Add focused tests for `src/audio/AudioFile.*`, `src/audio/SignalProcessing.*`, `src/acoustics/AcousticMetrics.*`, `src/acoustics/ImageSourceMethod.*`, and `src/acoustics/SimulationQueue.*`
- [x] Add focused tests for `src/rendering/Camera.*`, `src/rendering/MeshData.*`, `src/rendering/RayPicking.*`, `src/rendering/SurfaceGrouper.*`, and `src/rendering/TextureManager.*`
- [ ] Add headless GUI smoke tests for `src/gui/MainWindow.*`, `src/gui/dialogs/SettingsDialogs.*`, and `src/gui/widgets/ColorSwatch.*` to cover launch, open/save/load, and preference changes
- [ ] Document the coverage workflow and the 80% gate in `README.md` or `CONTRIBUTING.md`

## Phase 8 — DG Solver and Low-Frequency Accuracy

### Milestone: a clearer low-frequency alternative to the geometric solvers

> The DG code is present, but it needs a tighter integration story and better validation if it is to be a reliable option in the UI.

- [ ] Add a solver-selection abstraction that treats DG as a first-class render mode
- [ ] Add validation for 2D and 3D DG parameter ranges before starting a render
- [ ] Add comparison fixtures between DG, ISM, and ray-traced outputs in controlled rooms
- [ ] Document when to choose DG over ray tracing in the UI and user guide
- [ ] Add a small set of modal and low-frequency validation scenes

## Phase 9 — Metrics, Regression, and Comparison Workflow

### Milestone: stable audio comparisons and meaningful QA

> The current comparison window is useful, but the project needs stronger automatic regression coverage and more structured metrics.

- [ ] Add automated acoustic metric extraction for every rendered listener output
- [ ] Add baseline comparison files for representative rooms and source/listener placements
- [ ] Add command-line reports that summarize the delta between two renders
- [ ] Add regression tests for project loading, solver selection, and export
- [ ] Add a small corpus of golden `.wav` outputs for CI

## Phase 10 — Release and Distribution

### Milestone: easier to build, package, and share

> Once the core simulation path is hardened, packaging becomes the last-mile problem.

- [ ] Add reproducible release scripts for all supported platforms
- [ ] Bundle the materials library and example rooms with release artifacts
- [ ] Add version metadata to the application window and exported files
- [ ] Expand the user guide with solver selection, render workflow, and troubleshooting
- [ ] Add a release checklist that covers tests, docs, and packaging assets

## Phase 11 — Web Foundation

### Milestone: a browser build that launches and reuses the core app model

> This phase is about proving that Seiche can cross the Qt for WebAssembly boundary without rewriting the whole product. The goal is not feature parity yet; the goal is to get the editor booting in a browser with a browser-safe build and a narrow but useful core workflow.

#### Phase 11.1 — Toolchain and build support

- [x] Add a Qt for WebAssembly build target and document the required toolchain/version pairing
- [x] Add a browser-specific CMake preset or build script that produces a runnable `.html` + `.wasm` bundle
- [x] Define a minimal local run workflow for serving the Web build during development
- [x] Add CI notes or scripts for verifying that the Web build still configures after dependency changes

#### Phase 11.2 — Native dependency and runtime boundaries

- [ ] Split native-only code paths behind platform guards or build options
- [ ] Identify unsupported WebAssembly dependencies and provide browser-safe fallbacks or build exclusions
- [ ] Replace direct assumptions about writable local paths with an abstraction that can target browser storage or downloads
- [ ] Audit OpenGL usage and isolate code that cannot run under WebGL or WebAssembly context rules

#### Phase 11.3 — Browser editor bootstrap

- [ ] Launch the app in a browser with the main window or a browser-safe equivalent
- [ ] Load a minimal project or sample scene successfully in the browser
- [ ] Keep the core editor interactions working for selection, placement, and basic scene editing
- [ ] Make sure the browser build starts without requiring unsupported native dialogs or platform hooks

#### Phase 11.4 — Browser-safe import and export

- [ ] Replace native file-open flows with browser-compatible project and asset import flows
- [ ] Replace native file-save flows with browser-compatible download or export flows
- [ ] Preserve the ability to load room meshes, textures, and audio assets from browser-supported inputs
- [ ] Define a fallback story for assets that cannot be accessed as raw file paths in the browser

#### Phase 11.5 — Web smoke coverage

- [ ] Add a minimal WebAssembly smoke test for startup and resource loading
- [ ] Add a test or scripted check for opening a project and restoring the scene state in the browser build
- [ ] Add a test or scripted check for exporting a result artifact from the Web build
- [ ] Document the browser build limitations that remain at the end of this phase

## Phase 12 — Browser Workflow and Performance

### Milestone: the web build becomes a practical browser-first variant

> Once the app can start in the browser, the next step is making the workflow feel intentional instead of merely functional. This phase focuses on persistence, long-running work, and output handling in the constraints of the web platform.

#### Phase 12.1 — Browser persistence

- [ ] Store recent projects and UI preferences in browser-safe storage
- [ ] Restore the last-used browser state on reload when possible
- [ ] Define how project metadata survives across browser sessions
- [ ] Add a clear reset/clear-data path for browser storage

#### Phase 12.2 — Upload, download, and asset bundles

- [ ] Add explicit upload handling for project bundles and external assets
- [ ] Add explicit download handling for rendered audio, reports, and exports
- [ ] Define a packaging format for browser-friendly project bundles when raw file paths are unavailable
- [ ] Make the export story clear for single-file outputs versus multi-file output directories

#### Phase 12.3 — Background work and responsiveness

- [ ] Move long-running simulation work onto browser-safe background execution where supported
- [ ] Keep the UI responsive during loading, simulation setup, and export
- [ ] Add progress reporting that makes browser execution delays understandable to the user
- [ ] Define cancellation behavior for long-running browser tasks

#### Phase 12.4 — Unsupported-feature fallbacks

- [ ] Define fallback behavior for native folder selection, unrestricted local paths, and desktop audio device integration
- [ ] Replace any remaining desktop-only assumptions in dialogs, settings, and output workflows
- [ ] Make unsupported features fail gracefully with browser-specific messaging
- [ ] Document the browser capability gaps that remain after the phase is complete

#### Phase 12.5 — Web regression and performance checks

- [ ] Add browser regression fixtures for the editor workflow, simulation launch, and export path
- [ ] Add a small performance baseline for startup time and first-scene load time
- [ ] Add a smoke check that the browser build still round-trips a project bundle
- [ ] Document the scenarios that should be re-tested after browser-related changes

## Milestone Summary

| Milestone | Phases | Deliverable                          |
| --------- | ------ | ------------------------------------ |
| A         | 0      | Buildable, testable repo with CI     |
| B         | 7      | Coverage workflow and >80% gate      |
| C         | 1      | Stable project format and validation |
| D         | 2      | Headless rendering and export        |
| E         | 3      | Real binaural output                 |
| F         | 4      | Directivity-aware sources            |
| G         | 5      | Cleaner solver architecture          |
| H         | 6      | Working GPU acceleration             |
| I         | 8      | Better DG low-frequency accuracy     |
| J         | 9      | Regression and comparison workflow   |
| K         | 10     | Release packaging and docs           |
| L         | 11     | Browser build foundation             |
| M         | 12     | Browser workflow and performance     |
