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
The project is inspired by [pyroomacoustics](https://github.com/LCAV/pyroomacoustics), a library for testing and simulating acoustics algorithms written by Robin Scheibler.

> [!NOTE]
> PyRoomStudio is in the process of transition tech stacks from Python to C++ for improved performance. As such, the project/repo will be under a new name in the future.

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


## Setup

The latest build of PyRoomStudio is running on C++17 with Qt6 and Eigen3 dependencies.

1. Clone the repository

    ```
    $ git clone https://github.com/PyRoomStudio/PyRoomStudio.git
    ```

2. Install [Qt6](https://doc.qt.io/qt-6/qt-online-installation.html) (the open individual license is enough to compile the software). During installation, make sure to include the **MinGW** toolchain component under *Qt > Developer and Designer Tools*.

3. Compiling the software is done through Cmake/Ninja. The provided commands work on Windows/MacOS, but will require your system's specific Qt path.

    ```
    cmake -S . -B build \
        -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/mingw_64" \
        -DCMAKE_BUILD_TYPE=Release \
        -G Ninja
        
    cmake --build build
    ```

The version number can be different, although Qt >6 is expected. For MacOS specifically, replace `/mingw_64` with `/macos`.
Once Cmake has built the Ninja files, follow-up compilations will only need `cmake --build build`.

4. Run the `PyRoomStudio` executable.

```bash
$ ./build/PyRoomStudio
```

<!------------------------------------------------------------------->

## Controls

- Mouse: **[Left-click]** to select surfaces and points, **[Right-click]** to set surface color, **[Left-click + hold]** to rotate camera. 

- **[T]**: Set transparency on *all* surfaces of a loaded mesh.

<!------------------------------------------------------------------->

## Credits

- Big thanks to [Robin Scheibler](https://www.robinscheibler.org), [Zackery Belanger](https://empac.rpi.edu/program/people/researchers/zackery-belanger), [Mohamed Boubekri](https://arch.illinois.edu/people/profiles/mohamed-boubekri), and [Paris Smaragdis](https://www.mit.edu/~paris) for providing initial feedback and collaborating with us.

<!------------------------------------------------------------------->

<!-- <p align="center">
  <img width="700" src="assets/screenshot.png" alt="PyRoomStudio">
</p> -->
