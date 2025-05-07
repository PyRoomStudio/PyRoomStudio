"""
The Render2 Class processes all visual elements of the application using PyGame and PyOpenGL
to render meshes relative to the intended acoustics simulations.
"""

import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
import numpy as np
from stl import mesh
import math
import sys
from acoustic import Acoustic

# Constants for camera control
CAMERA_DIST = 5.0      # Distance from camera to object
MIN_DIST = 1.0         # Minimum zoom distance
MAX_DIST = 5.0         # Maximum zoom distance
CAMERA_HEADING = 35.0  # Horizontal rotation angle
CAMERA_PITCH = 35.0    # Vertical rotation angle
MIN_PITCH = 0.0        # Limit looking down
MAX_PITCH = 85.0       # Limit looking up

class Render:
    def __init__(self, filename, acoustic: Acoustic, width=800, height=600):
        """Initialize the renderer with PyGame and OpenGL"""
        pygame.init()
        self.width = width
        self.height = height
        self.display = pygame.display.set_mode((width, height), DOUBLEBUF | OPENGL)
        pygame.display.set_caption("3D Model Viewer")
        
        # Set up OpenGL
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_LIGHTING)
        glEnable(GL_LIGHT0)
        glEnable(GL_COLOR_MATERIAL)
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)
        
        # Set up the viewport
        glViewport(0, 0, width, height)
        glMatrixMode(GL_PROJECTION)
        gluPerspective(45, (width/height), 0.1, 50.0)
        glMatrixMode(GL_MODELVIEW)
        
        # Set white background
        glClearColor(1.0, 1.0, 1.0, 1.0)
        
        # Initialize camera parameters
        self.camera_distance = CAMERA_DIST
        self.camera_heading = CAMERA_HEADING
        self.camera_pitch = CAMERA_PITCH
        self.min_distance = MIN_DIST
        self.max_distance = MAX_DIST
        self.min_pitch = MIN_PITCH
        self.max_pitch = MAX_PITCH
        
        # Mouse control variables
        self.mouse_down = False
        self.last_mouse_pos = None
        
        # Load the model
        self.center, self.volume = self.compute_volumetric_properties(filename)
        self.ratio = (self.volume / 1000 / 7500)
        self.model = self.load_model(filename)
        
        # Store acoustic reference
        self.acoustic = acoustic
        
        # Set up lighting
        self.setup_lighting()
        
        # Initialize font for text rendering
        pygame.font.init()
        self.font = pygame.font.Font(None, 36)

    def compute_volumetric_properties(self, filename: str) -> tuple[np.ndarray, float]:
        """Computes the volumetric center (centroid) of a closed triangular mesh."""
        stl_mesh = mesh.Mesh.from_file(filename)
        
        total_volume = 0.0
        centroid_sum = np.zeros(3)
        
        for triangle in stl_mesh.vectors:
            v0, v1, v2 = triangle
            tetra_volume = np.dot(v0, np.cross(v1, v2)) / 6.0
            tetra_centroid = (v0 + v1 + v2) / 4.0
            
            centroid_sum += tetra_centroid * tetra_volume
            total_volume += tetra_volume
            
        if np.isclose(total_volume, 0):
            raise ValueError("Calculated volume is zero; ensure the STL mesh is closed and valid.")
            
        volumetric_center = centroid_sum / total_volume
        return volumetric_center, total_volume

    def load_model(self, filename):
        """Load a 3D model from file"""
        if filename.endswith('.stl'):
            stl_mesh = mesh.Mesh.from_file(filename)
            vertices = stl_mesh.vectors.reshape(-1, 3)
            normals = stl_mesh.normals
            return {'vertices': vertices, 'normals': normals}
        else:
            raise ValueError("Only .stl files are supported")

    def setup_lighting(self):
        """Set up scene lighting"""
        # Ambient light
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, [0.3, 0.3, 0.3, 1.0])
        
        # Main light
        glLightfv(GL_LIGHT0, GL_POSITION, [1, 1, 1, 0])
        glLightfv(GL_LIGHT0, GL_AMBIENT, [0.2, 0.2, 0.2, 1.0])
        glLightfv(GL_LIGHT0, GL_DIFFUSE, [0.8, 0.8, 0.8, 1.0])

    def update_camera(self):
        """Update camera position based on current angles and distance"""
        glLoadIdentity()
        
        # Convert angles to radians
        heading_rad = math.radians(self.camera_heading)
        pitch_rad = math.radians(self.camera_pitch)
        
        # Calculate camera position using spherical coordinates
        x = self.camera_distance * math.sin(heading_rad) * math.cos(pitch_rad)
        y = -self.camera_distance * math.cos(heading_rad) * math.cos(pitch_rad)
        z = self.camera_distance * math.sin(pitch_rad)
        
        gluLookAt(x, y, z, 0, 0, 0, 0, 0, 1)

    def draw_model(self):
        """Draw the loaded 3D model"""
        glPushMatrix()
        
        # Scale and center the model
        scale = 1/self.ratio
        glScalef(scale, scale, scale)
        newcenter = self.center/self.ratio
        glTranslatef(-newcenter[0], newcenter[1]*1.75, -newcenter[2])
        
        # Draw the model
        glBegin(GL_TRIANGLES)
        for i in range(0, len(self.model['vertices']), 3):
            normal = self.model['normals'][i//3]
            glNormal3fv(normal)
            for j in range(3):
                vertex = self.model['vertices'][i + j]
                glVertex3fv(vertex)
        glEnd()
        
        glPopMatrix()

    def draw_axes(self):
        """Draw coordinate axes"""
        glPushMatrix()
        glBegin(GL_LINES)
        
        # X axis (red)
        glColor3f(1, 0, 0)
        glVertex3f(0, 0, 0)
        glVertex3f(1, 0, 0)
        
        # Y axis (green)
        glColor3f(0, 1, 0)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 1, 0)
        
        # Z axis (blue)
        glColor3f(0, 0, 1)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 0, 1)
        
        glEnd()
        glPopMatrix()

    def draw_stats(self):
        """Draw camera statistics on screen"""
        # Create text surfaces with black color for better visibility on white background
        heading_text = f"Heading: {self.camera_heading:.1f}°"
        pitch_text = f"Pitch: {self.camera_pitch:.1f}°"
        
        heading_surface = self.font.render(heading_text, True, (0, 0, 0))
        pitch_surface = self.font.render(pitch_text, True, (0, 0, 0))
        
        # Switch to 2D rendering mode
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glLoadIdentity()
        glOrtho(0, self.width, self.height, 0, -1, 1)
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glLoadIdentity()
        
        # Disable depth testing and lighting for 2D rendering
        glDisable(GL_DEPTH_TEST)
        glDisable(GL_LIGHTING)
        
        # Enable blending for text
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        
        # Convert PyGame surface to OpenGL texture
        heading_data = pygame.image.tostring(heading_surface, "RGBA", True)
        pitch_data = pygame.image.tostring(pitch_surface, "RGBA", True)
        
        # Create and bind textures
        heading_texture = glGenTextures(1)
        pitch_texture = glGenTextures(1)
        
        # Upload heading texture
        glBindTexture(GL_TEXTURE_2D, heading_texture)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, heading_surface.get_width(), heading_surface.get_height(), 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, heading_data)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        
        # Upload pitch texture
        glBindTexture(GL_TEXTURE_2D, pitch_texture)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pitch_surface.get_width(), pitch_surface.get_height(), 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, pitch_data)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        
        # Enable texturing
        glEnable(GL_TEXTURE_2D)
        
        # Draw heading text (flipped texture coordinates to fix upside-down text)
        glBindTexture(GL_TEXTURE_2D, heading_texture)
        glBegin(GL_QUADS)
        glTexCoord2f(0, 1); glVertex2f(20, 20)
        glTexCoord2f(1, 1); glVertex2f(20 + heading_surface.get_width(), 20)
        glTexCoord2f(1, 0); glVertex2f(20 + heading_surface.get_width(), 20 + heading_surface.get_height())
        glTexCoord2f(0, 0); glVertex2f(20, 20 + heading_surface.get_height())
        glEnd()
        
        # Draw pitch text (flipped texture coordinates to fix upside-down text)
        glBindTexture(GL_TEXTURE_2D, pitch_texture)
        glBegin(GL_QUADS)
        glTexCoord2f(0, 1); glVertex2f(20, 60)
        glTexCoord2f(1, 1); glVertex2f(20 + pitch_surface.get_width(), 60)
        glTexCoord2f(1, 0); glVertex2f(20 + pitch_surface.get_width(), 60 + pitch_surface.get_height())
        glTexCoord2f(0, 0); glVertex2f(20, 60 + pitch_surface.get_height())
        glEnd()
        
        # Clean up
        glDeleteTextures([heading_texture, pitch_texture])
        glDisable(GL_TEXTURE_2D)
        
        # Restore OpenGL state
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()
        glMatrixMode(GL_MODELVIEW)
        glPopMatrix()
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_LIGHTING) 