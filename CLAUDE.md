# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**PyRoomStudio** is an open-source application for rendering and analyzing acoustical properties of 3D spaces via meshes. It provides a graphical interface for [pyroomacoustics](https://github.com/LCAV/pyroomacoustics), enabling users to visualize and simulate acoustic algorithms in 3D environments.

## Development Setup

### Environment Setup
```bash
# Clone and setup virtual environment
git clone https://github.com/ematth/PyRoomStudio.git
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
```

### Running the Application
```bash
python main_gui.py
```

## Architecture

### Core Components

**main_gui.py** - Entry point (20 lines)
- Minimal launcher that initializes pygame and imports MainApplication
- Sets application icon
- Runs the main application loop

**gui/** - Modular GUI package (8 files, ~2500 lines)
- **constants.py**: Color definitions and layout constants
- **base_components.py**: Base GUIComponent, TextButton, ImageButton, ToggleButton
- **input_components.py**: DropdownMenu, Slider, RadioButton, CheckBox
- **menu_components.py**: MenuItem, MenuButton, MenuBar
- **gallery_components.py**: ImageItem, SurfaceItem, ImageGallery, SurfaceGallery with scrolling
- **panel_components.py**: Panel, LibraryPanel (SOUND/MATERIAL tabs), PropertyPanel, AssetsPanel
- **application.py**: MainApplication class - coordinates GUI, 3D rendering, event handling
- **__init__.py**: Package exports for clean imports

**MainApplication** (gui/application.py:551)
- Built with PyGame and PyOpenGL
- Manages the 3D viewport and integrates with `Render3` for rendering
- Viewport area defined at initialization: `pygame.Rect(200, 90, width - 400, height - 220)`
- Loads STL/OBJ files via `load_stl_file()` method
- Routes events between GUI components and 3D renderer

**render3.py** - 3D rendering engine (~477 lines)
- `Render3` class handles all OpenGL rendering operations
- Surface detection and grouping: Uses BFS to group triangles into surfaces based on feature edges (angle threshold: 30°)
- Feature edge detection via `compute_feature_edges()` - identifies boundaries and sharp corners
- Camera system: orbital camera with heading, pitch, distance controls
- Ray-triangle intersection for mouse picking (Möller-Trumbore algorithm)
- Surface materials: supports both color and texture mapping per surface
- Texture mapping: projects textures based on surface normal direction (see `get_texture_coords_from_normal()`)
- `triangle_to_surface` dict maps each triangle index to its parent surface

**acoustic.py** - Acoustic simulation wrapper
- `Acoustic` class wraps pyroomacoustics functionality
- `SIZE_REDUCTION_FACTOR = 700.0` - scales mesh coordinates to realistic room dimensions
- `simulate()` method: configures room from mesh, places source/microphones, computes RIR (Room Impulse Response)
- Uses ray tracing (10,000 rays) and image source model for acoustic simulation
- Reads input from `sounds/piano.wav`, outputs to `sounds/output.wav`

### Key Data Flow

1. **Model Loading**: `main_gui.py` → `Render3(filename, view_rect, window_height)`
2. **Surface Detection**: STL mesh → feature edge detection → BFS grouping → surface list
3. **Rendering**: Each frame, GUI calls `renderer.draw_scene()` which sets viewport and renders model
4. **User Interaction**:
   - Left-click: Apply texture to surface (via ray picking)
   - Right-click: Change surface color
   - Mouse drag: Rotate camera
   - Scroll: Zoom camera
   - 'T' key: Toggle transparency on all surfaces
   - 'R' key: Reset all surfaces to default

### Surface System

The renderer groups mesh triangles into "surfaces" - contiguous regions bounded by feature edges:
- `surfaces`: List of sets, each set contains triangle indices for one surface
- `surface_colors`: RGB color for each surface (default: `[0.6, 0.8, 1.0]`)
- `surface_materials`: Texture state for each surface (None = color only, True = textured)
- `triangle_to_surface`: Reverse mapping from triangle index to surface index

## Dependencies

- **PyGame**: GUI and windowing
- **PyOpenGL**: 3D rendering
- **numpy-stl**: STL mesh loading
- **pyroomacoustics**: Acoustic simulation
- **Pillow (PIL)**: Texture loading
- **scipy**: Signal processing for acoustics

## Project Status

This project is under active development. Many fundamental features are not yet implemented.

## Resources

- Sample 3D models available in `resources/` directory (STL and OBJ formats)
- Authors: William (Zhiwen) Chen (willc@illinois.edu) & Evan M. Matthews (evanmm3@illinois.edu)
