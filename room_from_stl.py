"""
Random demo file for room acoustic simulation on an STL file.

References:
https://github.com/LCAV/pyroomacoustics/blob/master/notebooks/pyroomacoustics_demo.ipynb
https://github.com/LCAV/pyroomacoustics/blob/master/examples/room_from_stl.py
"""

import argparse
import os
from pathlib import Path
from scipy.io import wavfile
from scipy.signal import fftconvolve

import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits import mplot3d

import pyroomacoustics as pra

global size_reduc_factor
size_reduc_factor = 700.0  # to get a realistic room size (not 3km)

def compute_volumetric_center(stl_mesh):
    """
    Computes the volumetric center (centroid) of a closed triangular mesh.
    This assumes that the mesh is watertight.
    
    Parameters:
      stl_mesh (mesh.Mesh): The mesh object created using numpy-stl.
      
    Returns:
      np.ndarray: The 3D coordinates of the volumetric center.
    """
    total_volume = 0.0
    centroid_sum = np.zeros(3)

    # Iterate over each triangle in the mesh.
    for triangle in stl_mesh.vectors:
        # Unpack triangle vertices
        v0, v1, v2 = triangle

        # Compute the signed volume of the tetrahedron defined by (0, v0, v1, v2)
        # Using scalar triple product formula: V = dot(v0, cross(v1, v2)) / 6.
        tetra_volume = np.dot(v0, np.cross(v1, v2)) / 6.0

        # The centroid of the tetrahedron (including the origin 0,0,0) is given by:
        # (0 + v0 + v1 + v2) / 4 which simplifies to (v0 + v1 + v2) / 4.
        tetra_centroid = (v0 + v1 + v2) / 4.0

        centroid_sum += tetra_centroid * tetra_volume
        total_volume += tetra_volume

    # A quick sanity check: if total_volume is nearly zero, your mesh might not be closed.
    if np.isclose(total_volume, 0):
        raise ValueError("Calculated volume is zero; ensure the STL mesh is closed and valid.")

    # The overall centroid is the weighted average of the tetrahedron centroids.
    volumetric_center = centroid_sum / total_volume
    return volumetric_center / size_reduc_factor

try:
    from stl import mesh
except ImportError as err:
    print(
        "The numpy-stl package is required for this example. "
        "Install it with `pip install numpy-stl`"
    )
    raise err

default_stl_path = Path(__file__).parent / "resources/INRIA_MUSIS.stl"

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Basic room from STL file example")
    parser.add_argument(
        "--file", type=str, default=default_stl_path, help="Path to STL file"
    )
    args = parser.parse_args()

    material = pra.Material(energy_absorption=0.2, scattering=0.1)

    # with numpy-stl
    the_mesh = mesh.Mesh.from_file(args.file)
    ntriang, nvec, npts = the_mesh.vectors.shape

    # create one wall per triangle
    walls = []
    for w in range(ntriang):
        walls.append(
            pra.wall_factory(
                the_mesh.vectors[w].T / size_reduc_factor,
                material.energy_absorption["coeffs"],
                material.scattering["coeffs"],
            )
        )

    fs, signal = wavfile.read('sounds/piano.wav')
    signal = signal.astype(np.float32)/32768.0 # required.

    print('Room center: ', cent := compute_volumetric_center(the_mesh))

    room = (
        pra.Room(
            walls,
            fs=fs,
            max_order=3,
            ray_tracing=True,
            air_absorption=True,
        )
        .add_source(cent, signal=signal)
        .add_microphone_array(np.c_[cent + [0, 2, 0], cent + [0, -2, 0]])
    )

    print('Room volume: ', room.volume)

    # compute the rir
    # print('Image source model')
    # room.image_source_model()
    # print('Ray tracing')
    # room.ray_tracing()
    # print('Compute RIR')
    # room.compute_rir()
    # print('Plot RIR')
    # room.plot_rir()

    # print('simulate room')
    # room.simulate()

    # show the room
    room.plot(img_order=1)
    plt.show()

    # print(f'Shapes:\n Original:{signal.shape}\n Simulation:{room.mic_array.signals.shape}')

    # wavfile.write(filename='sounds/output.wav', rate=fs, data=room.mic_array.signals[0,:])

    # plt.plot(np.abs(signal), label='Original')
    # plt.plot(np.abs(room.mic_array.signals[0, :]), label='Simulated')
    # plt.title('Signal comparison')
    # plt.legend()
    # plt.show()