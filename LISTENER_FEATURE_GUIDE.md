# Listener Placement Feature - User Guide

## Overview
This feature allows you to place and control listener positions in the 3D scene for acoustic simulation.

## Features Implemented

### 1. Visual Listener Display
- Listeners are shown as **blue spheres** in the 3D viewport
- Selected listeners have a **yellow outline** for easy identification
- Sound sources are shown as **red spheres** (for future use)

### 2. Placing a Listener
1. Click the **"Place Listener"** button at the bottom of the screen
2. The window title changes to "Place Listener Mode"
3. Click on any surface in the 3D model to place the listener
4. The listener will be placed exactly where you clicked on the surface

### 3. Selecting a Listener
1. Click directly on a listener sphere in the 3D viewport
2. The selected listener will be highlighted with a yellow outline
3. Click elsewhere to deselect

### 4. Moving a Listener
Once a listener is selected, use keyboard controls:
- **Left Arrow** - Move left (X-)
- **Right Arrow** - Move right (X+)
- **Up Arrow** - Move forward (Y+)
- **Down Arrow** - Move backward (Y-)
- **Page Up** - Move up (Z+)
- **Page Down** - Move down (Z-)

Each keypress moves the listener by 5% of the model size.

### 5. Deleting a Listener
- Select a listener (click on it)
- Press **Delete** or **Backspace** to remove it

## Acoustic Simulation Integration

### Default Behavior
- If no listeners are placed, a default listener is automatically created at the room center
- If no sound sources are placed but a sound file is imported, a default source is created at the room center

### Using Placed Listeners
1. Place one or more listeners in desired locations
2. Import a sound file using **"Import Sound"** button
3. Click **"Render"** to run the acoustic simulation
4. Output files will be generated for each listener-source pair in `sounds/simulations/`

## Technical Details

### Listener Position
- Listeners can be placed on any surface of the 3D model
- Position is stored as a 3D vector [x, y, z] in world coordinates
- Positions are automatically scaled for acoustic simulation

### Scene Manager
- All listeners and sound sources are managed by the `SceneManager` class
- Supports multiple listeners and multiple sound sources
- Each combination generates a separate audio output file

## Example Workflow

1. **Load a 3D Model**
   - File → Open Project
   - Select an STL file

2. **Import Sound**
   - Click "Import Sound"
   - Select an audio file (.wav recommended)

3. **Place Listeners**
   - Click "Place Listener"
   - Click on desired locations in the model
   - Repeat to add multiple listeners

4. **Adjust Positions** (optional)
   - Click on a listener to select it
   - Use arrow keys to fine-tune position
   - Use Page Up/Down for vertical adjustment

5. **Run Simulation**
   - Click "Render"
   - Wait for simulation to complete
   - Find output files in `sounds/simulations/simulation_TIMESTAMP/`

## Console Output

When placing/moving listeners, you'll see console output like:
```
Placed listener at position: [1.234 5.678 9.012]
Selected listener 0 at [1.234 5.678 9.012]
Moved listener up to [1.234 5.678 9.512]
```

This helps you track exact positions for documentation or reproducibility.

## Known Limitations

- The old `acoustic.py` file still exists but is not used
- The new system uses `AcousticSimulator` with `SceneManager`
- Currently only supports WAV audio files for best compatibility

## Future Enhancements

Possible improvements for future versions:
- Drag-and-drop listener repositioning
- Visual indicators showing listener direction/orientation
- Preset listener configurations (e.g., audience seating layouts)
- Import/export listener positions as JSON
- Real-time audio preview while moving listeners
