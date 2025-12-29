<!-- <p align="center">
  <img width="180" src="assets/logo_v2.png" alt="PyRoomStudio">
  <p align="center"><b><font size=32px>PyRoomStudio</font></b></p>
</p> -->

<p align="center">
  <img width="400" src="assets/Logo_v3_color.svg" alt="PyRoomStudio">
</p>

---
<!------------------------------------------------------------------->

**PyRoomStudio** is an open-source application for rendering and analyzing acoustical properties of 3D spaces via meshes. 
The project acts as a method to graphically use [pyroomacoustics](https://github.com/LCAV/pyroomacoustics), a library for testing and simulating acoustics algorithms written by Robin Scheibler.

> [!NOTE]
> PyRoomStudio is still under significant development- a lot of fundamental features have not been fleshed out yet. Please join the [Discord](https://discord.gg/Q9ZtTcZZvH) server and make pull requests if you wish to contribute!



## **Get Involved!**

#### [Discord](https://discord.gg/Q9ZtTcZZvH)

#### Contacts:

William (Zhiwen) Chen (willc@illinois.edu), *Co-Founder & Designer*

Evan M. Matthews (evanmm3@illinois.edu), *Co-Founder & Progammer*


<!------------------------------------------------------------------->

## Mission Statement

PyRoomStudio has two missions:

1. Make room acoustics design accessible and intuitive for designers via open-source software.
2. Connect people passionate about designing for the auditory sense, from both the technical and the creative sides.

By accomplishing these missions, we hope to increase the usage of acoustic tools in the design community and make light of sound as a necessary consideration for future spaces.

<p align="center">
  <img width="600" src="assets/readme-image.png" alt="">
</p>


## Features

- **3D Model Import**: Load STL files to visualize architectural spaces
- **Interactive Camera**: Rotate, zoom, and navigate around your 3D model
- **Surface Customization**: Change colors and apply textures to surfaces
- **Listener Placement**: Click to place listeners anywhere on surfaces for acoustic analysis
- **Listener Control**: Move listeners in 3D space using keyboard controls
- **Multi-Source/Multi-Listener**: Support for multiple sound sources and listeners
- **Acoustic Simulation**: Powered by [pyroomacoustics](https://github.com/LCAV/pyroomacoustics) for realistic sound propagation
- **Audio Output**: Generate spatially-accurate audio files for each listener position

## Setup

PyRoomStudio is currently running on Python 3.9+. Make sure to have one of these versions installed before downloading and running the application.

1. Clone the repository

    ```
    $ git clone https://github.com/ematth/PyRoomStudio.git
    ```

2. Setup your virtual environment (venv) and install necessary packages

    ```
    $ python3 -m venv venv
    $ source venv/bin/activate
    (venv) $ pip install -r requirements.txt
    ```
3. Open the existing rendering GUI:

    ```
    $ python main_gui.py
    ```

<!------------------------------------------------------------------->

## Controls

### 3D Viewport
- Mouse: **[Left-click]** a surface to enable transparency, **[Right-click]** to set surface color, **[Left-click + hold]** to rotate camera. 

- **[T]**: Set transparency on *all* surfaces of a loaded mesh.

- **[R]**: Reset transparency and color on *all* surfaces.

### Listener Placement (New!)
- Click **"Place Listener"** button to enter placement mode
- **[Left-click]** on a surface to place a listener at that location
- **[Left-click]** on a listener sphere to select it
- Use **Arrow Keys** to move selected listener horizontally
- Use **Page Up/Down** to move selected listener vertically
- Press **Delete** to remove selected listener

See [LISTENER_FEATURE_GUIDE.md](LISTENER_FEATURE_GUIDE.md) for detailed instructions.

<!------------------------------------------------------------------->

## Credits

- Big thanks to [Robin Scheibler](https://www.robinscheibler.org), [Zackery Belanger](https://empac.rpi.edu/program/people/researchers/zackery-belanger), [Mohamed Boubekri](https://arch.illinois.edu/people/profiles/mohamed-boubekri), and [Paris Smaragdis](https://www.mit.edu/~paris) for providing initial feedback and collaborating with us.

<!------------------------------------------------------------------->

<p align="center">
  <img width="700" src="assets/screenshot.png" alt="PyRoomStudio">
</p>
