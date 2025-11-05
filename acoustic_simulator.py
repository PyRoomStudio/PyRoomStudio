"""
Acoustic Simulator - Bridge between GUI and pyroomacoustics.
Handles multi-source, multi-listener acoustic simulations.
"""

import pyroomacoustics as pra
import numpy as np
from scipy.io import wavfile
from typing import List, Tuple, Optional
from datetime import datetime
import os
import json

from scene_manager import SceneManager, SoundSource, Listener

# Same scaling factor as acoustic.py
SIZE_REDUCTION_FACTOR = 700.0


class AcousticSimulator:
    """
    Manages acoustic simulations with multiple sound sources and listeners.
    """

    def __init__(self, sample_rate: int = 44100):
        self.sample_rate = sample_rate
        self.speed_of_sound = 343.0
        self.last_simulation_dir = None

    def simulate_scene(
        self,
        scene_manager: SceneManager,
        walls_from_render: List[dict],
        room_center: np.ndarray,
        model_vertices: np.ndarray,
        max_order: int = 3,
        n_rays: int = 10000,
        energy_absorption: float = 0.2,
        scattering: float = 0.1,
    ) -> Optional[str]:
        """
        Simulate acoustic scene with multiple sources and listeners.

        Args:
            scene_manager: SceneManager containing sources and listeners
            walls_from_render: Wall information from 3D renderer
            room_center: Center of the room in world coordinates
            model_vertices: Vertices of the 3D model
            max_order: Maximum order for image source model
            n_rays: Number of rays for ray tracing
            energy_absorption: Material energy absorption coefficient
            scattering: Material scattering coefficient

        Returns:
            Path to the simulation output directory, or None if simulation failed
        """
        # Validate scene has minimum requirements
        if not scene_manager.has_minimum_objects():
            print("ERROR: Need at least 1 sound source and 1 listener")
            return None

        print(f"\n{'='*60}")
        print("ACOUSTIC SIMULATION START")
        print(f"{'='*60}")
        print(f"Sound sources: {len(scene_manager.sound_sources)}")
        print(f"Listeners: {len(scene_manager.listeners)}")
        print(f"Max order: {max_order}, Rays: {n_rays}")
        print(f"Energy absorption: {energy_absorption}, Scattering: {scattering}")

        # Create output directory with timestamp
        timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        output_dir = os.path.join("sounds", "simulations", f"simulation_{timestamp}")
        os.makedirs(output_dir, exist_ok=True)
        print(f"Output directory: {output_dir}")

        # Save scene configuration
        scene_config_path = os.path.join(output_dir, "scene.json")
        scene_manager.save_to_file(scene_config_path)
        print(f"Saved scene configuration to {scene_config_path}")

        # Create material
        material = pra.Material(energy_absorption=energy_absorption, scattering=scattering)

        # Build walls list from renderer's triangle data
        print("\nBuilding room geometry...")
        walls = []
        for wall_info in walls_from_render:
            for tri_idx in wall_info['triangles']:
                triangle_verts = model_vertices[tri_idx * 3 : tri_idx * 3 + 3]

                if triangle_verts.shape[0] < 3:
                    continue

                # Scale down to realistic room size
                pra_vertices = triangle_verts.T / SIZE_REDUCTION_FACTOR

                walls.append(
                    pra.wall_factory(
                        pra_vertices,
                        material.energy_absorption["coeffs"],
                        material.scattering["coeffs"],
                    )
                )

        print(f"Created {len(walls)} wall triangles")

        # For each sound source, run a simulation
        for source_idx, sound_source in enumerate(scene_manager.sound_sources):
            print(f"\n{'-'*60}")
            print(f"Processing sound source {source_idx + 1}/{len(scene_manager.sound_sources)}")
            print(f"  Name: {sound_source.name}")
            print(f"  Audio: {sound_source.audio_file}")
            print(f"  Position: {sound_source.position}")

            # Load source audio
            if not os.path.exists(sound_source.audio_file):
                print(f"  ERROR: Audio file not found: {sound_source.audio_file}")
                continue

            try:
                fs, signal = wavfile.read(sound_source.audio_file)

                # Convert to mono if stereo
                if len(signal.shape) > 1:
                    signal = signal.mean(axis=1)

                # Normalize to float32 [-1, 1]
                if signal.dtype == np.int16:
                    signal = signal.astype(np.float32) / 32768.0
                elif signal.dtype == np.int32:
                    signal = signal.astype(np.float32) / 2147483648.0
                else:
                    signal = signal.astype(np.float32)

                # Apply volume
                signal = signal * sound_source.volume

                print(f"  Loaded audio: {signal.shape[0]} samples, {fs} Hz")

            except Exception as e:
                print(f"  ERROR loading audio: {e}")
                continue

            # Scale source position
            source_pos_scaled = sound_source.position / SIZE_REDUCTION_FACTOR

            # Prepare microphone array from all listeners
            mic_positions = []
            for listener in scene_manager.listeners:
                mic_pos_scaled = listener.position / SIZE_REDUCTION_FACTOR
                mic_positions.append(mic_pos_scaled)

            # Convert to column vectors for pyroomacoustics (shape: 3, n_mics)
            mic_array = np.column_stack(mic_positions)
            print(f"  Microphone array shape: {mic_array.shape} ({len(mic_positions)} mics)")

            # Create room
            print(f"  Creating room...")
            room = pra.Room(
                walls,
                fs=fs,
                max_order=max_order,
                ray_tracing=True,
                air_absorption=True,
            )

            # Add source
            room.add_source(source_pos_scaled, signal=signal)

            # Add microphone array
            room.add_microphone_array(mic_array)

            # Set ray tracing parameters
            room.set_ray_tracing(n_rays=n_rays)

            print(f"  Room volume: {room.volume:.2f} m³")

            # Run simulation
            try:
                print(f"  Running image source model...")
                room.image_source_model()

                print(f"  Running ray tracing...")
                room.ray_tracing()

                print(f"  Computing room impulse responses...")
                room.compute_rir()

                print(f"  Simulating sound propagation...")
                room.simulate()

                print(f"  Simulation complete!")

                # Save output for each listener
                for listener_idx, listener in enumerate(scene_manager.listeners):
                    output_signal = room.mic_array.signals[listener_idx, :]

                    # Generate output filename
                    listener_name = listener.name.replace(" ", "_")
                    source_name = sound_source.name.replace(" ", "_")
                    output_filename = f"{listener_name}_from_{source_name}.wav"
                    output_path = os.path.join(output_dir, output_filename)

                    # Write output audio
                    output_signal_int16 = np.int16(output_signal * 32767)
                    wavfile.write(output_path, fs, output_signal_int16)
                    print(f"    Saved: {output_filename}")

            except Exception as e:
                print(f"  ERROR during simulation: {e}")
                import traceback
                traceback.print_exc()
                continue

        print(f"\n{'='*60}")
        print("SIMULATION COMPLETE")
        print(f"Output directory: {os.path.abspath(output_dir)}")
        print(f"{'='*60}\n")

        self.last_simulation_dir = output_dir
        return output_dir

    def get_last_simulation_dir(self) -> Optional[str]:
        """Get the directory of the last simulation"""
        return self.last_simulation_dir
