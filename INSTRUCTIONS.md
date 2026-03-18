# INSTRUCTIONS

This document will guide you through all the controls and features of the software.

## Opening Projects

To start a new project, use **ctrl + N** or go to File -> "New Project", and select a 3D mesh. Currently, the accepted file types are `.stl` and `.obj`. Saved work will be stored as a `.room` file, keeping track of mesh scaling, source/listening points, materials, and other details of your workflow. These files can be opened later using **ctrl + O** or by going to File -> "Open Project". You can also see recently opened projects in File -> Recent Projects.

## Controls

To move around the rendered space, **hold left-click and drag the mouse**, and use the **scrollwheel** to zoom in and out. The camera acts as a sphere around your object, allowing you to view your work from all sides.

**Left-click** allows you to "select" surfaces or points for modification, while left-clicking off the mesh de-selects the surface or point.

Additionally, a standard set ctrl-shortcuts are avaiable with **ctrl+key**, the Edit dropdown menu, or the tool banner just below the dropdown menus. These include: cut, copy, paste, delete, undo, and redo. Undo and Redo are applicable to every action you can perform on the mesh, while cut, copy, paste, and delete are specifically for points.

Move and Measure are also provided, albeit only in the tool banner. *Move* allows a point to be moved with respect to the camera's position. *Measure* computes the distance after left-clicking the measurement's start and end points. Currently, distances are printed in the bottom left of the application window.

## Settings

### Preferences

This sections allows you to choose a default folder to save project files to, along with toggling auto-save and choosing an auto-save interval in minutes.

### Display

This sections includes options for visual elements within the rendered space. *Show measurement grid* and *grid minor spacing* toggle and change the spacing of grid squares under the mesh. *Transparency alpha* determines how transparent surfaces are while in **T-mode**. Lastly, *point marker size* increases or decreases the size of source/listener points. 

### Audio

Currently, we only have the option of changing the output *sample rate* from a select list. Output files are in 16-bit PCM WAV format.

### Simulation

- *Max ISM Order*: The maximum order from the Image-Source Method, i.e. the total number of surface reflections to simulate from each ray.
- *Number of Rays*: Number of rays to simulate, in all directions, from each source point.
- *Default absorption*: The percentage of sound absorbed for surfaces with no defined absorption coefficient.
- *Default Scatting*: The percentage of sound energy scattered by surfaces with no defined scattering coefficient.
- *Enable air absorption*: Toggle to include 20% air absorption for all rays. Useful for simulating spaces with no defined surface materials.

### Keyboard Shortcuts

This section shows a list of all available shortcuts.
