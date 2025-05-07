"""
The Render Class processes all visual elements of the application by 
rendering meshes relative to the intended acoustics simulations.
"""


from direct.showbase.ShowBase import ShowBase, loadPrcFileData
from panda3d.core import *
import math, sys, simplepbr, os.path as path
from direct.gui.OnscreenText import OnscreenText
import numpy as np
from stl import mesh
from acoustic import Acoustic

# Enable the assimp loader so that .obj files can be loaded.
loadPrcFileData("", "load-file-type p3assimp")

CAMERA_DIST: float = 5.0      # Distance from camera to object
MIN_DIST: float = 1.0          # Minimum zoom distance
MAX_DIST: float = 5.0         # Maximum zoom distance
CAMERA_HEADING: float = 35.0   # Horizontal rotation angle
CAMERA_PITCH: float = 35.0     # Vertical rotation angle
MIN_PITCH: float = 0.0         # Limit looking down
MAX_PITCH: float = 85.0        # Limit looking up

class Render(ShowBase):

    def compute_volumetric_properties(self, filename: str) -> tuple[np.ndarray, float]:
        """
        Computes the volumetric center (centroid) of a closed triangular mesh.
        This assumes that the mesh is watertight.
        
        Parameters:
        stl_mesh (mesh.Mesh): The mesh object created using numpy-stl.
        
        Returns:
        np.ndarray: The 3D coordinates of the volumetric center.
        """
        stl_mesh = mesh.Mesh.from_file(filename)

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
        return volumetric_center, total_volume

    def load_base(self, name) -> None:
        """Loads a file from a default described in Panda3D

        Args:
            name (_type_): name of the object, i.e. teapot, etc.
            options: teapot, jack, ripple, box, frowney, environment, 
            cmtt12, cmss12, cmr12, camera, shuttle_controls, yup-axis, zup-axis.
        """
        try:
            self.model = self.loader.loadModel(name)
            self.model.reparentTo(self.render)

            # Center and scale the model to fit the scene
            self.model.setScale(1/self.ratio)
            newcenter = self.center/self.ratio
            self.model.setPos(-newcenter[0], newcenter[1]*1.75, -newcenter[2])
            self.model.setHpr(0, 90, 0)
            # self.model.setPos(0, 0, -0.5)
            # self.model.setScale(0.5)
            # self.model.setHpr(0, 90, 0)
            # self.model.setTransparency(TransparencyAttrib.MAlpha)
            # self.model.setAlphaScale(0.8)

        except:
            print('Unable to load model. Please make sure that the model type exists.')


    def model_loader(self, filename) -> None:
        """Match the file extension and load the model accordingly.

        Args:
            filename (str): name of the model file.
        """
        match filename[-4:] in ['.obj', '.stl']:
            case True:
                self.load_base(filename)
            case _:
                print('Invalid file format. Only .obj files are supported.')
                sys.exit()


    def create_floor(self):
        """Create a floor grid beneath the teapot"""
        # Create a CardMaker to make a flat card
        cm = CardMaker('floor')
        cm.setFrame(-10, 10, -10, 10)  # 20x20 unit floor
        
        # Create the floor and attach it to the scene
        floor = self.render.attachNewNode(cm.generate())
        floor.setP(-90)  # Rotate it to be horizontal (pitch -90 degrees)
        floor.setZ(-0.5)  # Position slightly below the origin to avoid z-fighting
        floor.setColor(0.5, 1.0, 0.5, 1)  # Set the color to light gray


    def axes_indicator(self) -> None:
        """Renders axes indicator to bottom corner of the screen"""
        corner = self.aspect2d.attachNewNode("corner of screen")
        corner.setPos(-1.2, 0, -0.9)
        self.axis = self.loader.loadModel("zup-axis")
        self.axis.setScale(0.025)
        self.axis.reparentTo(corner)


    def axisTask(self, task):
        self.axis.setHpr(self.camera.getHpr())
        return task.cont


    def __init__(self, filename, acoustic: Acoustic) -> None:
        ShowBase.__init__(self)

        simplepbr.init()

        self.setFrameRateMeter(True)
        self.frameRateMeter.setUpdateInterval(0.1)

        # Set background color
        self.setBackgroundColor(0.1, 0.1, 0.2)

        # Compute volume for model scaling
        self.center, self.volume = self.compute_volumetric_properties(filename)
        self.ratio = (self.volume/ 1000 / 7500)
        print(f"Center: {self.center}\n Volume: {self.volume}")
        print(f"New Center: {self.center/self.ratio}\n New Volume: {self.volume/self.ratio}")
        print('Ratio: ', self.volume / 1000 / 7500)
    

        # Load the model and floor
        #self.create_floor()
        self.axes_indicator()
        self.model_loader(filename)

        # Give access to the acoustic class
        self.acoustic = acoustic

        # Set up lighting
        self.setup_lighting()
        
        # Disable the default camera control system
        self.disableMouse()

        # normals rendering boolean
        self.normals: bool = False
        
        # Set up camera parameters
        self.camera_distance = CAMERA_DIST       # Distance from camera to object
        self.min_distance = MIN_DIST          # Minimum zoom distance
        self.max_distance = MAX_DIST         # Maximum zoom distance
        self.camera_heading = CAMERA_HEADING        # Horizontal rotation angle
        self.camera_pitch = CAMERA_PITCH          # Vertical rotation angle
        self.min_pitch = 0.0           # Limit looking down
        self.max_pitch = 85.0            # Limit looking up
        
        # Initialize mouse state tracking
        self.mouse_x = 0
        self.mouse_y = 0
        self.last_mouse_x = 0
        self.last_mouse_y = 0
        self.mouse_button_1 = False
        
        # Set up mouse event handlers
        self.accept("mouse1", self.on_mouse_down)
        self.accept("mouse1-up", self.on_mouse_up)
        self.accept("wheel_up", self.on_wheel_up)
        self.accept("wheel_down", self.on_wheel_down)
        self.accept("escape", sys.exit)
        self.accept("u", self.obj_info)
        self.accept("f", self.flip_model)
        # self accept "p" keybind to send to the acoustic class
        self.accept("p", self.acoustic.simulate)
        # self.accept("r", self.show_planenormals)

        # Add the update task.
        self.taskMgr.add(self.update_camera, "UpdateCameraTask")
        self.taskMgr.add(self.axisTask, "axisTask")

        # Create onscreen text for camera stats
        self.heading_text = OnscreenText(
            text="Heading: 0.0°",
            pos=(-1.3, 0.7),
            scale=0.07,
            fg=(1, 1, 1, 1),
            align=TextNode.ALeft
        )
        
        self.pitch_text = OnscreenText(
            text="Pitch: 0.0°",
            pos=(-1.3, 0.6),
            scale=0.07,
            fg=(1, 1, 1, 1),
            align=TextNode.ALeft
        )
        
        # Initialize camera position.
        self.update_camera_position()


    # def show_planenormals(self) -> None:
    #     # If the normals are already shown, remove them
    #     if self.normals:
    #         for arrow in self.render.findAllMatches('**/camera'):
    #             arrow.removeNode()
    #         self.normals = False
    #         return
    #     # Draw arrows to show the normals of each plane
    #     else:
    #         for plane in self.model.findAllMatches('**/+GeomNode'):
    #             geom_node = plane.node()
    #             for i in range(geom_node.getNumGeoms()):
    #                 geom = geom_node.getGeom(i)
    #                 vdata = geom.getVertexData()
    #                 normal_reader = GeomVertexReader(vdata, 'normal')
    #                 vertex_reader = GeomVertexReader(vdata, 'vertex')

    #                 for j in range(vdata.getNumRows()):
    #                     normal_reader.setRow(j)
    #                     vertex_reader.setRow(j)
    #                     normal = normal_reader.getData3()
    #                     vertex = vertex_reader.getData3()

    #                     # Create an arrow to represent the normal
    #                     arrow = self.loader.loadModel("camera")
    #                     arrow.setScale(0.1)
    #                     arrow.setPos(vertex[0], vertex[1], vertex[2])
    #                     arrow.lookAt(vertex[0] + normal[0], vertex[1] + normal[1], vertex[2] + normal[2])
    #                     arrow.reparentTo(self.render)
    #                     arrow.setColor(1, 0, 0, 1)  # Set color to red
    #                     arrow.setTransparency(TransparencyAttrib.MAlpha)
    #                     arrow.setAlphaScale(0.5)  # Set transparency
    #                     arrow.setLightOff()  # Disable lighting for the arrow  
    #       self.normals = True


    def flip_model(self) -> None:
        "Flips the model by some factor of 90 degrees"
        hpr = self.model.getHpr()
        #print(hpr, hpr[0], hpr[1], hpr[2], type(hpr))
        self.model.setHpr(0, hpr[1] + 90, hpr[2])


    # see https://github.com/LCAV/pyroomacoustics/issues/392
    def obj_info(self) -> str:
        """Returns the object information"""
        geom_nodes = self.model.findAllMatches('**/+GeomNode')
        for node_path in geom_nodes:
            geom_node = node_path.node()
            print(f"\n[GeomNode: {geom_node.getName()}]")

            for i in range(geom_node.getNumGeoms()):
                geom = geom_node.getGeom(i)
                vdata = geom.getVertexData()

                # Create readers for each attribute
                vertex_reader = GeomVertexReader(vdata, 'vertex')
                normal_reader = GeomVertexReader(vdata, 'normal') if vdata.hasColumn('normal') else None
                color_reader = GeomVertexReader(vdata, 'color') if vdata.hasColumn('color') else None

                print(f"  Geom {i}: {vdata.getNumRows()} vertices")

                for j in range(vdata.getNumRows()):
                    vertex_reader.setRow(j)
                    vertex = vertex_reader.getData3()
                    print(f"    Vertex {j}: {vertex}")

                    if normal_reader:
                        normal_reader.setRow(j)
                        normal = normal_reader.getData3()
                        print(f"      Normal: {normal}")

                    if color_reader:
                        color_reader.setRow(j)
                        color = color_reader.getData4()
                        print(f"      Color: {color}")

        return self.model.getName()


    def setup_lighting(self):
        """Set up basic scene lighting"""
        # Add ambient light
        ambient_light = AmbientLight("ambient")
        ambient_light.setColor(Vec4(0.3, 0.3, 0.3, 1))
        ambient_np = self.render.attachNewNode(ambient_light)
        self.render.setLight(ambient_np)
        
        # Add directional light (key light)
        main_light = DirectionalLight("main_light")
        main_light.setColor(Vec4(0.8, 0.8, 0.8, 1))
        main_light_np = self.render.attachNewNode(main_light)
        main_light_np.setHpr(45, -30, 0)
        self.render.setLight(main_light_np)
        
        # Add another directional light for fill
        fill_light = DirectionalLight("fill_light")
        fill_light.setColor(Vec4(0.4, 0.4, 0.5, 1))
        fill_light_np = self.render.attachNewNode(fill_light)
        fill_light_np.setHpr(-45, 20, 0)
        self.render.setLight(fill_light_np)


    def on_mouse_down(self):
        """Handler for mouse button down event"""
        if self.mouseWatcherNode.hasMouse():
            # Get current mouse position
            self.last_mouse_x = self.mouseWatcherNode.getMouseX()
            self.last_mouse_y = self.mouseWatcherNode.getMouseY()
            self.mouse_button_1 = True
    

    def on_mouse_up(self):
        """Handler for mouse button up event"""
        self.mouse_button_1 = False
    

    def on_wheel_up(self):
        """Handler for mouse wheel up (zoom in)"""
        self.camera_distance = max(self.min_distance, self.camera_distance - 0.5)
        self.update_camera_position()
    

    def on_wheel_down(self):
        """Handler for mouse wheel down (zoom out)"""
        self.camera_distance = min(self.max_distance, self.camera_distance + 0.5)
        self.update_camera_position()
    

    def update_camera(self, task):
        """Task to update camera based on mouse input"""
        if self.mouse_button_1 and self.mouseWatcherNode.hasMouse():
            # Get current mouse position
            mouse_x = self.mouseWatcherNode.getMouseX()
            mouse_y = self.mouseWatcherNode.getMouseY()
            
            # Calculate the mouse movement
            dx = mouse_x - self.last_mouse_x
            dy = mouse_y - self.last_mouse_y
            
            # Update camera angles (scale the movement for better control)
            self.camera_heading -= dx * 100.0
            self.camera_pitch += dy * 100.0

            self.camera_heading = self.camera_heading % 360.0
            
            # Clamp the pitch to prevent flipping
            self.camera_pitch = min(max(self.camera_pitch, self.min_pitch), self.max_pitch)
            
            # Store current mouse position for next frame
            self.last_mouse_x = mouse_x
            self.last_mouse_y = mouse_y
            
            # Update the camera position
            self.update_camera_position()

            self.update_stats_display()
        
        return task.cont
    

    def update_stats_display(self):
        """Update the onscreen text with current camera stats"""
        self.heading_text.setText(f"Heading: {self.camera_heading:.1f}°")
        self.pitch_text.setText(f"Pitch: {self.camera_pitch:.1f}°")
    

    def update_camera_position(self):
        """Update camera position based on current angles and distance"""
        # Convert angles to radians
        heading_rad = math.radians(self.camera_heading)
        pitch_rad = math.radians(self.camera_pitch)
        
        # Calculate camera position using spherical coordinates
        x = self.camera_distance * math.sin(heading_rad) * math.cos(pitch_rad)
        y = -self.camera_distance * math.cos(heading_rad) * math.cos(pitch_rad)
        z = self.camera_distance * math.sin(pitch_rad)
        
        # Update camera position and make it look at the model
        self.camera.setPos(x, y, z)
        self.camera.lookAt(Point3(0, 0, 0))