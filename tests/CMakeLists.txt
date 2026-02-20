find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_acoustic_simulation test_acoustic_simulation.cpp
    ${CMAKE_SOURCE_DIR}/src/acoustics/AcousticSimulator.cpp
    ${CMAKE_SOURCE_DIR}/src/acoustics/ImageSourceMethod.cpp
    ${CMAKE_SOURCE_DIR}/src/acoustics/RayTracer.cpp
    ${CMAKE_SOURCE_DIR}/src/acoustics/RoomImpulseResponse.cpp
    ${CMAKE_SOURCE_DIR}/src/acoustics/Wall.cpp
    ${CMAKE_SOURCE_DIR}/src/audio/AudioFile.cpp
    ${CMAKE_SOURCE_DIR}/src/audio/SignalProcessing.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/RayPicking.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/MeshData.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/SurfaceGrouper.cpp
    ${CMAKE_SOURCE_DIR}/src/scene/SceneManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Material.cpp
)

target_include_directories(test_acoustic_simulation PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(test_acoustic_simulation PRIVATE
    Qt6::Core
    Qt6::Test
    Eigen3::Eigen
)

add_test(NAME AcousticSimulation COMMAND test_acoustic_simulation)
