"""
Scene Manager for PyRoomStudio.
Manages sound sources, listeners, and their 3D positions in the scene.
"""

import numpy as np
from typing import List, Optional, Tuple
from dataclasses import dataclass
import json
from datetime import datetime


@dataclass
class SoundSource:
    """Represents a sound source in the 3D scene"""
    position: np.ndarray  # [x, y, z] in world coordinates
    audio_file: str  # Path to audio file
    volume: float = 1.0  # Volume multiplier (0.0 to 1.0)
    name: str = ""  # User-friendly name
    marker_color: Tuple[int, int, int] = (255, 100, 100)  # Red-ish

    def __post_init__(self):
        """Ensure position is numpy array"""
        if not isinstance(self.position, np.ndarray):
            self.position = np.array(self.position, dtype=np.float32)
        if not self.name:
            self.name = f"Sound Source {id(self) % 1000}"

    def to_dict(self):
        """Serialize to dictionary for saving"""
        return {
            'position': self.position.tolist(),
            'audio_file': self.audio_file,
            'volume': self.volume,
            'name': self.name,
            'marker_color': self.marker_color
        }

    @classmethod
    def from_dict(cls, data):
        """Deserialize from dictionary"""
        return cls(
            position=np.array(data['position'], dtype=np.float32),
            audio_file=data['audio_file'],
            volume=data.get('volume', 1.0),
            name=data.get('name', ''),
            marker_color=tuple(data.get('marker_color', (255, 100, 100)))
        )


@dataclass
class Listener:
    """Represents a listener/microphone in the 3D scene"""
    position: np.ndarray  # [x, y, z] in world coordinates
    name: str = ""  # User-friendly name
    orientation: Optional[np.ndarray] = None  # Optional direction vector for directional mics
    marker_color: Tuple[int, int, int] = (100, 100, 255)  # Blue-ish

    def __post_init__(self):
        """Ensure position is numpy array"""
        if not isinstance(self.position, np.ndarray):
            self.position = np.array(self.position, dtype=np.float32)
        if self.orientation is not None and not isinstance(self.orientation, np.ndarray):
            self.orientation = np.array(self.orientation, dtype=np.float32)
        if not self.name:
            self.name = f"Listener {id(self) % 1000}"

    def to_dict(self):
        """Serialize to dictionary for saving"""
        return {
            'position': self.position.tolist(),
            'name': self.name,
            'orientation': self.orientation.tolist() if self.orientation is not None else None,
            'marker_color': self.marker_color
        }

    @classmethod
    def from_dict(cls, data):
        """Deserialize from dictionary"""
        orientation = data.get('orientation')
        if orientation is not None:
            orientation = np.array(orientation, dtype=np.float32)
        return cls(
            position=np.array(data['position'], dtype=np.float32),
            name=data.get('name', ''),
            orientation=orientation,
            marker_color=tuple(data.get('marker_color', (100, 100, 255)))
        )


class SceneManager:
    """Manages the acoustic scene with sound sources and listeners"""

    def __init__(self):
        self.sound_sources: List[SoundSource] = []
        self.listeners: List[Listener] = []
        self.selected_source_index: Optional[int] = None
        self.selected_listener_index: Optional[int] = None

    def add_sound_source(self, position: np.ndarray, audio_file: str,
                        volume: float = 1.0, name: str = "") -> SoundSource:
        """Add a sound source to the scene"""
        source = SoundSource(position, audio_file, volume, name)
        self.sound_sources.append(source)
        print(f"Added sound source at {position} with audio: {audio_file}")
        return source

    def add_listener(self, position: np.ndarray, name: str = "",
                    orientation: Optional[np.ndarray] = None) -> Listener:
        """Add a listener to the scene"""
        listener = Listener(position, name, orientation)
        self.listeners.append(listener)
        print(f"Added listener at {position}")
        return listener

    def remove_sound_source(self, index: int) -> bool:
        """Remove a sound source by index"""
        if 0 <= index < len(self.sound_sources):
            removed = self.sound_sources.pop(index)
            print(f"Removed sound source: {removed.name}")
            if self.selected_source_index == index:
                self.selected_source_index = None
            elif self.selected_source_index is not None and self.selected_source_index > index:
                self.selected_source_index -= 1
            return True
        return False

    def remove_listener(self, index: int) -> bool:
        """Remove a listener by index"""
        if 0 <= index < len(self.listeners):
            removed = self.listeners.pop(index)
            print(f"Removed listener: {removed.name}")
            if self.selected_listener_index == index:
                self.selected_listener_index = None
            elif self.selected_listener_index is not None and self.selected_listener_index > index:
                self.selected_listener_index -= 1
            return True
        return False

    def clear_all(self):
        """Remove all sources and listeners"""
        self.sound_sources.clear()
        self.listeners.clear()
        self.selected_source_index = None
        self.selected_listener_index = None
        print("Cleared all sound sources and listeners")

    def get_sound_source(self, index: int) -> Optional[SoundSource]:
        """Get a sound source by index"""
        if 0 <= index < len(self.sound_sources):
            return self.sound_sources[index]
        return None

    def get_listener(self, index: int) -> Optional[Listener]:
        """Get a listener by index"""
        if 0 <= index < len(self.listeners):
            return self.listeners[index]
        return None

    def has_minimum_objects(self) -> bool:
        """Check if scene has minimum required objects for simulation"""
        return len(self.sound_sources) >= 1 and len(self.listeners) >= 1

    def get_all_positions(self) -> Tuple[List[np.ndarray], List[np.ndarray]]:
        """Get all source and listener positions"""
        source_positions = [source.position for source in self.sound_sources]
        listener_positions = [listener.position for listener in self.listeners]
        return source_positions, listener_positions

    def select_source(self, index: Optional[int]):
        """Select a sound source (for editing/deletion)"""
        if index is None or 0 <= index < len(self.sound_sources):
            self.selected_source_index = index
            self.selected_listener_index = None  # Deselect listeners

    def select_listener(self, index: Optional[int]):
        """Select a listener (for editing/deletion)"""
        if index is None or 0 <= index < len(self.listeners):
            self.selected_listener_index = index
            self.selected_source_index = None  # Deselect sources

    def get_selected_object(self) -> Optional[Tuple[str, int]]:
        """Get the currently selected object type and index"""
        if self.selected_source_index is not None:
            return ("source", self.selected_source_index)
        elif self.selected_listener_index is not None:
            return ("listener", self.selected_listener_index)
        return None

    def delete_selected(self) -> bool:
        """Delete the currently selected object"""
        if self.selected_source_index is not None:
            return self.remove_sound_source(self.selected_source_index)
        elif self.selected_listener_index is not None:
            return self.remove_listener(self.selected_listener_index)
        return False

    def save_to_file(self, filepath: str):
        """Save scene to JSON file"""
        data = {
            'version': '1.0',
            'timestamp': datetime.now().isoformat(),
            'sound_sources': [source.to_dict() for source in self.sound_sources],
            'listeners': [listener.to_dict() for listener in self.listeners]
        }
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
        print(f"Saved scene to {filepath}")

    def load_from_file(self, filepath: str):
        """Load scene from JSON file"""
        with open(filepath, 'r') as f:
            data = json.load(f)

        self.clear_all()

        # Load sound sources
        for source_data in data.get('sound_sources', []):
            source = SoundSource.from_dict(source_data)
            self.sound_sources.append(source)

        # Load listeners
        for listener_data in data.get('listeners', []):
            listener = Listener.from_dict(listener_data)
            self.listeners.append(listener)

        print(f"Loaded scene from {filepath}: {len(self.sound_sources)} sources, {len(self.listeners)} listeners")

    def get_summary(self) -> str:
        """Get a text summary of the scene"""
        return (f"Scene: {len(self.sound_sources)} sound source(s), "
                f"{len(self.listeners)} listener(s)")
