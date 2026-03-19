# INSTRUCTIONS

This document will guide you through all the controls and features of the software.

## Opening Projects

To start a new project, use **ctrl + N** or go to File -> "New Project", and select a 3D mesh. Currently, the accepted file types are `.stl` and `.obj`. Saved work will be stored as a `.room` file, keeping track of mesh scaling, source/listening points, materials, and other details of your workflow. These files can be opened later using **ctrl + O** or by going to File -> "Open Project". You can also see recently opened projects in File -> Recent Projects.

## Simulation Workflow

It's important to understand the application workflow before jumping into fine details.
Every project or `.room` file consists of surfaces and points. 

**Surfaces** describes walls, floors and ceilings that can reflect and absorb sound. A surface can have a **material** which dictates the amount of absorption and reflection for that surface, both in the range of 0 to 100%. To interact with surfaces, simply **left-click** the desired surface for modification, and left-click away from the mesh space to de-select the surface.

**Points** describe both source and listening objects that exist in the space. **source points** act as speakers that output sound in all directions, while **listening points** act as "simulated heads" which take input in left and right channels. Additionlly, the direction of listening points (i.e. where someone is facing) is indicated by an arrow on the point. Points can be interacted through **left-click** and **ctrl+A**. To add new points, press **P** or click "Place Point" in the bottom tool bar. To position a point, select a point on an existing surface, then use the **scroll-wheel** or "Point Distance" in the properties side menu to change the distance perpendicular to said surface. This mode can be toggled off by pressing **P** again or by clicking "Stop Placing" in the bottom tool bar. Lastly, points take priority when selecting something in the mesh space, even if the point is on the other side of several surfaces.

By default, surfaces are non-transparent, which makes point placement difficult. For a clearer point view, press **T** for *Transparency Mode*.

## Library Panel

On the left, the **Library Panel** gives you access to available sounds and materials to apply to your working space. For sound sources, click "Browse Folder" to find and select your sounds folder.
For materials, `.mat` files are stored and loaded in the `materials` folder, with sub-folders to organize materials.

To use sounds and materials, simply select the appropriate object, then double-click the appropriate item. Surfaces will take on the material's identifying color/texture and appear by named in the "Selected Surface" box in the Property Panel, while sound sources will appear in the point's "Selected Point" box in the Property Panel. Additionally, you can directly click-and-drag materials to surfaces.

## Property Panel

The **Property Panel** contains options for changing fine details with your mesh and selected objects. 

- *Room Scale*: Adjusts the size of the room by volume, while maintaining surface and point proportions. 
- *Point Distance*: Adjusts the perpendicular distance from the surface from which the point was placed.
- *Selected Point*: Properties for the selected point, including naming, identifying as a *source* of *listener*. Source points have options for selecting an output sound and sound volume, while Listening points have a "Facing Direction" for rotating the left and right input channels.
- *Selected Surface*: Provides material and texture options for the selected surface.

## Assets Panel

The **Assets** Panel provides a gallery containing all available surfaces on the mesh. These surfaces can be **left-click** selected from the panel to access any mesh surface. Optionally, you can un-check the box by the mesh's file name to disable selection from this panel.

## Controls

To move around the rendered space, **hold left-click and drag the mouse**, and use the **scrollwheel** to zoom in and out. The camera acts as a sphere around your object, allowing you to view your work from all directions.

**Left-click** allows you to "select" surfaces or points for modification, while left-clicking off the mesh de-selects the surface or point.

Additionally, a standard set ctrl-shortcuts are avaiable with **ctrl+key**, the Edit dropdown menu, or the tool banner just below the dropdown menus. These include: cut, copy, paste, delete, undo, and redo. Undo and Redo are applicable to every action you can perform on the mesh, while cut, copy, paste, and delete are specifically for points.

Move and Measure are also provided, albeit only in the tool banner. *Move* allows a point to be moved with respect to the camera's position. *Measure* computes the distance after left-clicking the measurement's start and end points. Currently, distances are printed in the bottom left of the application window.

## Space Simulation

Once at least one source point (with output file) and one listener point are placed, you can press "Render" in the bottom tool bar to open the "Render Options" popup. This details methods for simulating the space, along with selecting which listeners to simulate for. Each listening point gets its own rendered output, which currently is a two-channel `.wav` file.

Rendering a series of listening points places them in the *Simulation Queue*, where progress can be monitored. Some or all simulations can be cancelled in this window with "Cancel Current" and "Cancel All", respectively. Lastly, completing a simulation will open a pop-up to open the batch of simulations in the *Comparison Menu*, where playback, A/B Comparison and Acoustic Metrics can be found. This window can be re-opened by double-clicking the batch in the Simulation Queue.

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

This section shows a list of all available shortcuts. Currently, shortcuts are not modifiable.
