cmake_minimum_required(VERSION 3.16)
project(PyRoomStudio VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Multimedia Svg)

# Eigen3: try system install first, fall back to FetchContent
find_package(Eigen3 3.3 QUIET)
if(NOT Eigen3_FOUND)
    message(STATUS "Eigen3 not found on system -- downloading via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG        3.4.0
        GIT_SHALLOW    TRUE
    )
    set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(eigen)
endif()

find_package(OpenMP)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SNDFILE sndfile)
    pkg_check_modules(FFTW3 fftw3f)
endif()

if(NOT SNDFILE_FOUND)
    find_library(SNDFILE_LIBRARIES NAMES sndfile libsndfile-1)
    find_path(SNDFILE_INCLUDE_DIRS sndfile.h)
endif()

if(NOT FFTW3_FOUND)
    find_library(FFTW3_LIBRARIES NAMES fftw3f libfftw3f-3)
    find_path(FFTW3_INCLUDE_DIRS fftw3.h)
endif()

set(SOURCES
    src/main.cpp
    src/core/Material.cpp
    src/core/MaterialLoader.cpp
    src/core/SoundSource.cpp
    src/core/Listener.cpp
    src/core/PlacedPoint.cpp
    src/core/ProjectFile.cpp
    src/scene/SceneManager.cpp
    src/rendering/Viewport3D.cpp
    src/rendering/Camera.cpp
    src/rendering/MeshData.cpp
    src/rendering/SurfaceGrouper.cpp
    src/rendering/RayPicking.cpp
    src/rendering/MeshSimplifier.cpp
    src/rendering/TextureManager.cpp
    src/acoustics/AcousticSimulator.cpp
    src/acoustics/Bvh.cpp
    src/acoustics/ImageSourceMethod.cpp
    src/acoustics/RayTracer.cpp
    src/acoustics/RoomImpulseResponse.cpp
    src/acoustics/Wall.cpp
    src/acoustics/SimulationWorker.cpp
    src/acoustics/SimulationQueue.cpp
    src/acoustics/AcousticMetrics.cpp
    src/acoustics/dg/DGBasis2D.cpp
    src/acoustics/dg/DGBasis3D.cpp
    src/acoustics/dg/DGMesh2D.cpp
    src/acoustics/dg/DGMesh3D.cpp
    src/acoustics/dg/DGAcoustics2D.cpp
    src/acoustics/dg/DGAcoustics3D.cpp
    src/acoustics/dg/DGSolver.cpp
    src/acoustics/dg/DGGpuCompute.cpp
    src/audio/AudioFile.cpp
    src/audio/SignalProcessing.cpp
    src/gui/MainWindow.cpp
    src/gui/LibraryPanel.cpp
    src/gui/PropertyPanel.cpp
    src/gui/AssetsPanel.cpp
    src/gui/BottomToolbar.cpp
    src/gui/UndoCommands.cpp
    src/gui/SimulationQueuePanel.cpp
    src/gui/dialogs/SettingsDialogs.cpp
    src/gui/dialogs/AudioComparisonDialog.cpp
    src/gui/dialogs/RenderOptionsDialog.cpp
    src/gui/widgets/MaterialGallery.cpp
    src/gui/widgets/SurfaceGallery.cpp
    src/gui/widgets/ColorSwatch.cpp
    src/utils/ResourcePath.cpp
)

set(HEADERS
    src/core/Types.h
    src/core/Material.h
    src/core/MaterialLoader.h
    src/core/SoundSource.h
    src/core/Listener.h
    src/core/PlacedPoint.h
    src/core/ProjectFile.h
    src/scene/SceneManager.h
    src/rendering/GLHeaders.h
    src/rendering/Viewport3D.h
    src/rendering/Camera.h
    src/rendering/MeshData.h
    src/rendering/SurfaceGrouper.h
    src/rendering/RayPicking.h
    src/rendering/MeshSimplifier.h
    src/rendering/TextureManager.h
    src/acoustics/AcousticSimulator.h
    src/acoustics/Bvh.h
    src/acoustics/ImageSourceMethod.h
    src/acoustics/RayTracer.h
    src/acoustics/RoomImpulseResponse.h
    src/acoustics/Wall.h
    src/acoustics/SimulationWorker.h
    src/acoustics/SimulationQueue.h
    src/acoustics/AcousticMetrics.h
    src/acoustics/dg/DGTypes.h
    src/acoustics/dg/DGBasis2D.h
    src/acoustics/dg/DGBasis3D.h
    src/acoustics/dg/DGMesh2D.h
    src/acoustics/dg/DGMesh3D.h
    src/acoustics/dg/DGAcoustics2D.h
    src/acoustics/dg/DGAcoustics3D.h
    src/acoustics/dg/DGSolver.h
    src/acoustics/dg/DGGpuCompute.h
    src/audio/AudioFile.h
    src/audio/SignalProcessing.h
    src/gui/MainWindow.h
    src/gui/LibraryPanel.h
    src/gui/PropertyPanel.h
    src/gui/AssetsPanel.h
    src/gui/BottomToolbar.h
    src/gui/UndoCommands.h
    src/gui/SimulationQueuePanel.h
    src/gui/dialogs/SettingsDialogs.h
    src/gui/dialogs/AudioComparisonDialog.h
    src/gui/dialogs/RenderOptionsDialog.h
    src/gui/widgets/MaterialGallery.h
    src/gui/widgets/SurfaceGallery.h
    src/gui/widgets/ColorSwatch.h
    src/utils/ResourcePath.h
)

qt6_add_resources(RESOURCES resources/resources.qrc)

if(WIN32)
    set(APP_RC resources/app.rc)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES} ${APP_RC})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES} ${APP_RC})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Multimedia
    Qt6::Svg
    Eigen3::Eigen
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()

if(SNDFILE_LIBRARIES)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SNDFILE_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SNDFILE_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_SNDFILE)
endif()

if(FFTW3_LIBRARIES)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW3_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FFTW3_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_FFTW3)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32 glu32)
elseif(APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL OpenGL::GLU)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL OpenGL::GLU)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:QT_QML_DEBUG>
    _USE_MATH_DEFINES
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

enable_testing()
add_subdirectory(tests)
