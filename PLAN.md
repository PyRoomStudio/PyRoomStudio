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

## Phase X — Coverage Foundation and High-Risk Gaps

### Milestone: measurable repo-wide coverage above 80%

> The existing suite exercises the main paths, but the coverage target is not yet enforced or evenly distributed. This phase makes coverage measurable first, then pushes the weak spots over the threshold with focused tests instead of broad refactors.

> Current baseline from `gcovr` on `src/` sources, excluding tests and generated MOC/compiler-id files: 23.4% line coverage, 32.0% function coverage, and 12.8% branch coverage.

- [x] Add a repeatable Linux coverage job using `gcovr` that reports `src/` line coverage, excludes tests and generated files, and fails the build when coverage drops below 80%
- [x] Add unit tests for `src/core/MaterialLoader.*`, `src/core/ProjectFile.*`, and `src/utils/ResourcePath.*` to cover malformed inputs, missing files, and canonical round-trips
- [ ] Add headless GUI smoke tests for `src/gui/MainWindow.*`, `src/gui/dialogs/SettingsDialogs.*`, and `src/gui/widgets/ColorSwatch.*` to cover launch, open/save/load, and preference changes
- [ ] Add focused tests for `src/audio/AudioFile.*`, `src/audio/SignalProcessing.*`, `src/acoustics/AcousticMetrics.*`, `src/acoustics/ImageSourceMethod.*`, and `src/acoustics/SimulationQueue.*`
- [ ] Add focused tests for `src/rendering/Camera.*`, `src/rendering/MeshData.*`, `src/rendering/RayPicking.*`, `src/rendering/SurfaceGrouper.*`, and `src/rendering/TextureManager.*`
- [ ] Document the coverage workflow and the 80% gate in `README.md` or `CONTRIBUTING.md`

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

- [ ] Introduce an HRTF dataset abstraction for listener rendering
- [ ] Add head-orientation-aware direction lookup before convolution
- [ ] Add binaural output modes for mono, stereo, and per-listener export
- [ ] Add a synthetic HRTF fixture so binaural tests can run without external SOFA files
- [ ] Add a UI path for selecting an HRTF dataset per listener

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

## Phase 7 — DG Solver and Low-Frequency Accuracy

### Milestone: a clearer low-frequency alternative to the geometric solvers

> The DG code is present, but it needs a tighter integration story and better validation if it is to be a reliable option in the UI.

- [ ] Add a solver-selection abstraction that treats DG as a first-class render mode
- [ ] Add validation for 2D and 3D DG parameter ranges before starting a render
- [ ] Add comparison fixtures between DG, ISM, and ray-traced outputs in controlled rooms
- [ ] Document when to choose DG over ray tracing in the UI and user guide
- [ ] Add a small set of modal and low-frequency validation scenes

## Phase 8 — Metrics, Regression, and Comparison Workflow

### Milestone: stable audio comparisons and meaningful QA

> The current comparison window is useful, but the project needs stronger automatic regression coverage and more structured metrics.

- [ ] Add automated acoustic metric extraction for every rendered listener output
- [ ] Add baseline comparison files for representative rooms and source/listener placements
- [ ] Add command-line reports that summarize the delta between two renders
- [ ] Add regression tests for project loading, solver selection, and export
- [ ] Add a small corpus of golden `.wav` outputs for CI

## Phase 9 — Release and Distribution

### Milestone: easier to build, package, and share

> Once the core simulation path is hardened, packaging becomes the last-mile problem.

- [ ] Add reproducible release scripts for all supported platforms
- [ ] Bundle the materials library and example rooms with release artifacts
- [ ] Add version metadata to the application window and exported files
- [ ] Expand the user guide with solver selection, render workflow, and troubleshooting
- [ ] Add a release checklist that covers tests, docs, and packaging assets

## Milestone Summary

| Milestone | Phases | Deliverable                          |
| --------- | ------ | ------------------------------------ |
| A         | 0      | Buildable, testable repo with CI     |
| B         | X      | Coverage workflow and >80% gate      |
| C         | 1      | Stable project format and validation |
| D         | 2      | Headless rendering and export        |
| E         | 3      | Real binaural output                 |
| F         | 4      | Directivity-aware sources            |
| G         | 5      | Cleaner solver architecture          |
| H         | 6      | Working GPU acceleration             |
| I         | 7      | Better DG low-frequency accuracy     |
| J         | 8      | Regression and comparison workflow   |
| K         | 9      | Release packaging and docs           |
