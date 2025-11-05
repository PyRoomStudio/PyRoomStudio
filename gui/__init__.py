"""
PyRoomStudio GUI Package

A modular GUI framework for 3D acoustic simulation and visualization.
Provides components for building interactive 3D applications with PyGame and OpenGL.
"""

# Import core constants
from .constants import Colors

# Import base components
from .base_components import (
    GUIComponent,
    TextButton,
    ImageButton,
    ToggleButton
)

# Import input components
from .input_components import (
    DropdownMenu,
    Slider,
    RadioButton,
    RadioButtonGroup,
    CheckBox
)

# Import menu components
from .menu_components import (
    MenuItem,
    MenuButton,
    MenuBar
)

# Import gallery components
from .gallery_components import (
    ImageItem,
    SurfaceItem,
    ImageGallery,
    SurfaceGallery
)

# Import panel components
from .panel_components import (
    Panel,
    LibraryPanel,
    PropertyPanel,
    AssetsPanel
)

# Import main application
from .application import MainApplication

# Define public API
__all__ = [
    # Constants
    'Colors',
    
    # Base Components
    'GUIComponent',
    'TextButton',
    'ImageButton',
    'ToggleButton',
    
    # Input Components
    'DropdownMenu',
    'Slider',
    'RadioButton',
    'RadioButtonGroup',
    'CheckBox',
    
    # Menu Components
    'MenuItem',
    'MenuButton',
    'MenuBar',
    
    # Gallery Components
    'ImageItem',
    'SurfaceItem',
    'ImageGallery',
    'SurfaceGallery',
    
    # Panel Components
    'Panel',
    'LibraryPanel',
    'PropertyPanel',
    'AssetsPanel',
    
    # Main Application
    'MainApplication',
]

__version__ = '1.0.0'
__author__ = 'PyRoomStudio Team'
