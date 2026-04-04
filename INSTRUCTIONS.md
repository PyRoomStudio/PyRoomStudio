# INSTRUCTIONS

This document will guide you through all the controls and features of the Seiche software.

## Opening Projects

To start a new project, use **ctrl + N** or go to File -> "New Project", and select a 3D mesh. Currently, the accepted file types are `.stl` and `.obj`. Saved work will be stored as a `.room` file, keeping track of mesh scaling, source/listening points, materials, and other details of your workflow. These files can be opened later using **ctrl + O** or by going to File -> "Open Project". You can also see recently opened projects in File -> Recent Projects.

## Simulation Workflow

It's important to understand the application workflow before jumping into fine details.
Every project or `.room` file consists of surfaces and points.

**Surfaces** describes walls, floors and ceilings that can reflect and absorb sound. A surface can have a **material** which dictates the amount of absorption and reflection for that surface, both in the range of 0 to 100%. To interact with surfaces, simply **left-click** the desired surface for modification, and left-click away from the mesh space to de-select the surface.

**Points** describe both source and listening objects that exist in the space. **source points** act as speakers that output sound in all directions, while **listening points** act as "simulated heads" which take input in left and right channels. Additionlly, the direction of listening points (i.e. where someone is facing) is indicated by an arrow on the point. Points can be interacted through **left-click** and **ctrl+A**. To add new points, press **P** (cycles: off → add sources → add listeners → off) or use **Add Source** / **Add Listener** in the bottom tool bar. To position a point, select a point on an existing surface, then use the **scroll-wheel** or "Point Distance" in the properties side menu to change the distance perpendicular to said surface. Click the active add button again to stop placement. **Left-click** away from points and surfaces clears the current selection (including after **ctrl+A** select-all). Lastly, points take priority when selecting something in the mesh space, even if the point is on the other side of several surfaces.

By default, surfaces are non-transparent, which makes point placement difficult. For a clearer point view, press **T** for _Transparency Mode_.

## Library Panel

On the left, the **Library Panel** gives you access to available sounds and materials to apply to your working space. For sound sources, click "Browse Folder" to find and select your sounds folder.
For materials, `.mat` files are stored and loaded in the `materials` folder, with sub-folders to organize materials.

To use sounds and materials, simply select the appropriate object, then double-click the appropriate item. Surfaces will take on the material's identifying color/texture and appear by named in the "Selected Surface" box in the Property Panel, while sound sources will appear in the point's "Selected Point" box in the Property Panel. Additionally, you can directly click-and-drag materials to surfaces.

## Property Panel

The **Property Panel** shows one context at a time: room options when nothing is selected, point options when a point is selected, or surface options when a surface is selected.

- _Room Scale_ (room context): Adjusts the size of the room by volume, while maintaining surface and point proportions.
- _Point Distance_ (point context): Adjusts the perpendicular distance from the surface from which the point was placed.
- _Selected Point_ (point context): Naming, type as _source_ or _listener_, audio and volume for sources, and "Facing Direction" for listeners.
- _Selected Surface_ (surface context): Material and texture options for the selected surface.

## Assets Panel

The **Assets** Panel uses tabs similar to the Library: **SURFACES** lists all mesh surfaces (click to select); **SOURCES** and **LISTENERS** list placed points by name (click to select that point in the viewport).

## Controls

To move around the rendered space, **hold left-click and drag the mouse**, and use the **scrollwheel** to zoom in and out. The camera acts as a sphere around your object, allowing you to view your work from all directions.

**Left-click** allows you to "select" surfaces or points for modification, while left-clicking off the mesh de-selects the surface or point.

Additionally, a standard set ctrl-shortcuts are avaiable with **ctrl+key**, the Edit dropdown menu, or the tool banner just below the dropdown menus. These include: cut, copy, paste, delete, undo, and redo. Undo and Redo are applicable to every action you can perform on the mesh, while cut, copy, paste, and delete are specifically for points.

Move and Measure are also provided, albeit only in the tool banner. _Move_ allows a point to be moved with respect to the camera's position. _Measure_ computes the distance after left-clicking the measurement's start and end points. Currently, distances are printed in the bottom left of the application window.

## Space Simulation

Once at least one source point (with output file) and one listener point are placed, you can press "Render" in the bottom tool bar to open the "Render Options" popup. This details methods for simulating the space, along with selecting which listeners to simulate for. Each listening point gets its own rendered output, which currently is a two-channel `.wav` file.

Rendering a series of listening points places them in the _Simulation Queue_ (open from **View → Simulation Queue**). Progress can be monitored there; use "Cancel Current" and "Cancel All" to stop jobs. Completing a simulation will offer to open the _Comparison Menu_ for playback, A/B comparison, and acoustic metrics. Double-click a completed batch in the queue to open comparisons again.

## Settings

### Preferences

This sections allows you to choose a default folder to save project files to, along with toggling auto-save and choosing an auto-save interval in minutes.

### Display

This sections includes options for visual elements within the rendered space. _Show measurement grid_ and _grid minor spacing_ toggle and change the spacing of grid squares under the mesh. _Transparency alpha_ determines how transparent surfaces are while in **T-mode**. _Point marker size_ increases or decreases the size of source/listener points (and affects click targeting). _Solid colors only_ draws surfaces without textures, using their material colors only.

### Audio

Currently, we only have the option of changing the output _sample rate_ from a select list. Output files are in 16-bit PCM WAV format.

### Simulation

- _Max ISM Order_: The maximum order from the Image-Source Method, i.e. the total number of surface reflections to simulate from each ray.
- _Number of Rays_: Number of rays to simulate, in all directions, from each source point.
- _Default absorption_: The percentage of sound absorbed for surfaces with no defined absorption coefficient.
- _Default Scatting_: The percentage of sound energy scattered by surfaces with no defined scattering coefficient.
- _Enable air absorption_: Toggle to include 20% air absorption for all rays. Useful for simulating spaces with no defined surface materials.

### Keyboard Shortcuts

This section shows a list of all available shortcuts. Currently, shortcuts are not modifiable.
