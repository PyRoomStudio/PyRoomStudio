"""
Panel GUI components: Panel, LibraryPanel, PropertyPanel, AssetsPanel
"""
import pygame
import os
import random
from typing import List, Tuple, Optional, Callable
from .constants import Colors
from .base_components import GUIComponent, TextButton
from .input_components import DropdownMenu, Slider, RadioButtonGroup, CheckBox
from .gallery_components import ImageGallery, SurfaceGallery, MaterialGallery


class Panel(GUIComponent):
    """A container for organizing GUI elements"""
    
    def __init__(self, x: int, y: int, width: int, height: int, 
                 title: str = "", collapsible: bool = False):
        super().__init__(x, y, width, height)
        self.title = title
        self.collapsible = collapsible
        self.collapsed = False
        self.components: List[GUIComponent] = []
        self.font = pygame.font.Font(None, 16)
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.title_bg_color = Colors.LIGHT_GRAY
        self.title_text_color = Colors.BLACK
        
        # Title bar height
        self.title_height = 25 if title else 0
        self.content_rect = pygame.Rect(x, y + self.title_height, width, height - self.title_height)
    
    def add_component(self, component: GUIComponent):
        """Add a component to this panel"""
        self.components.append(component)
    
    def handle_event(self, event: pygame.event.Event) -> bool:
        if not self.visible or not self.enabled:
            return False
        
        # Handle title bar click for collapsible panels
        if self.collapsible and self.title and event.type == pygame.MOUSEBUTTONDOWN:
            title_rect = pygame.Rect(self.rect.x, self.rect.y, self.rect.width, self.title_height)
            if title_rect.collidepoint(event.pos):
                self.collapsed = not self.collapsed
                return True
        
        # Handle component events if not collapsed
        if not self.collapsed:
            for component in self.components:
                if component.handle_event(event):
                    return True
        
        return False
    
    def update(self, dt: float):
        if not self.collapsed:
            for component in self.components:
                component.update(dt)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Draw panel background
        pygame.draw.rect(surface, self.bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 2)
        
        # Draw title bar
        if self.title:
            title_rect = pygame.Rect(self.rect.x, self.rect.y, self.rect.width, self.title_height)
            pygame.draw.rect(surface, self.title_bg_color, title_rect)
            pygame.draw.rect(surface, self.border_color, title_rect, 1)
            
            # Draw title text
            title_surface = self.font.render(self.title, True, self.title_text_color)
            title_text_rect = title_surface.get_rect(midleft=(title_rect.x + 5, title_rect.centery))
            surface.blit(title_surface, title_text_rect)
            
            # Draw collapse indicator
            if self.collapsible:
                arrow_x = title_rect.right - 15
                arrow_y = title_rect.centery
                if self.collapsed:
                    # Right arrow
                    arrow_points = [(arrow_x - 3, arrow_y - 5), (arrow_x + 3, arrow_y), (arrow_x - 3, arrow_y + 5)]
                else:
                    # Down arrow
                    arrow_points = [(arrow_x - 5, arrow_y - 3), (arrow_x + 5, arrow_y - 3), (arrow_x, arrow_y + 3)]
                pygame.draw.polygon(surface, self.title_text_color, arrow_points)
        
        # Draw components if not collapsed
        if not self.collapsed:
            # Create a clipping rect for the content area
            content_clip = surface.get_clip()
            surface.set_clip(self.content_rect)
            
            for component in self.components:
                component.draw(surface)
            
            surface.set_clip(content_clip)


# Acoustic material data: {category: [(material_name, absorption_coeff, color), ...]}
# Absorption coefficients are in the range (0, 1) where:
#   0 = fully reflective (no absorption)
#   1 = fully absorptive (complete absorption)
# Colors are RGB tuples (0-255)

def _generate_material_color(seed: int) -> Tuple[int, int, int]:
    """Generate a deterministic random color based on a seed"""
    random.seed(seed)
    return (random.randint(60, 220), random.randint(60, 220), random.randint(60, 220))

ACOUSTIC_MATERIALS = {
    "Wall Materials": [
        ("Brick", 0.03, (178, 102, 85)),       # Brick red-brown
        ("Concrete", 0.02, (160, 160, 160)),   # Gray
        ("Drywall", 0.10, (240, 235, 225)),    # Off-white
        ("Plaster", 0.04, (245, 240, 230)),    # Cream
        ("Glass", 0.03, (200, 230, 245)),      # Light blue tint
        ("Wood Panel", 0.10, (160, 120, 80)),  # Wood brown
    ],
    "Floor Materials": [
        ("Hardwood", 0.10, (140, 90, 55)),     # Dark wood
        ("Carpet Thick", 0.65, (100, 80, 120)),# Deep purple
        ("Carpet Thin", 0.35, (150, 130, 160)),# Light purple
        ("Tile", 0.02, (210, 200, 180)),       # Beige
        ("Linoleum", 0.03, (180, 200, 170)),   # Light green
        ("Concrete", 0.02, (145, 145, 145)),   # Dark gray
    ],
    "Ceiling Materials": [
        ("Acoustic", 0.70, (90, 90, 110)),     # Dark blue-gray
        ("Plaster", 0.04, (250, 250, 245)),    # White
        ("Wood", 0.10, (180, 140, 100)),       # Light wood
        ("Metal", 0.05, (190, 195, 200)),      # Silver
        ("Suspended", 0.50, (220, 220, 230)),  # Light gray
    ],
    "Soft Furnish": [
        ("Heavy Drape", 0.55, (130, 50, 70)),  # Burgundy
        ("Light Drape", 0.15, (230, 200, 180)),# Cream
        ("Upholstery", 0.45, (80, 100, 130)),  # Navy blue
        ("Fabric", 0.60, (140, 160, 80)),      # Olive green
    ],
}


class LibraryPanel(GUIComponent):
    """A specialized panel for the library with SOUND/MATERIAL tabs"""
    
    def __init__(self, x: int, y: int, width: int, height: int):
        super().__init__(x, y, width, height)
        self.font = pygame.font.Font(None, 16)
        self.tab_font = pygame.font.Font(None, 14)
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.header_bg_color = Colors.LIGHT_GRAY
        self.header_text_color = Colors.BLACK
        self.tab_active_color = Colors.WHITE
        self.tab_inactive_color = Colors.LIGHT_GRAY
        self.tab_text_color = Colors.BLACK
        
        # Layout
        self.header_height = 25
        self.tab_height = 25
        self.content_y = self.rect.y + self.header_height + self.tab_height
        self.content_height = self.rect.height - self.header_height - self.tab_height
        
        # Tab system
        self.active_tab = "SOUND"  # "SOUND" or "MATERIAL"
        self.tab_width = 80
        
        # Reference to the renderer (set by application)
        self.renderer = None
        
        # Galleries for each tab
        self.sound_galleries: List[ImageGallery] = []
        self.material_galleries: List[MaterialGallery] = []
        
        self._create_sample_galleries()
    
    def _create_sample_galleries(self):
        """Create sample galleries with placeholder data"""
        # Sound galleries
        voices_items = [
            ("assets/adult_male.png", "Adult Male"),
            ("assets/adult_female.png", "Adult Female"),
            ("assets/adult_male.png", "Young boy"),
            ("assets/adult_female.png", "Young girl"),
        ]
        
        voices_gallery = ImageGallery(self.rect.x + 5, self.content_y + 5, 
                                    self.rect.width - 10, "Voices", voices_items,
                                    tooltip="Future feature!")
        voices_gallery.enabled = False  # Disable individual items
        self.sound_galleries.append(voices_gallery)
        
        # Material galleries - populated from ACOUSTIC_MATERIALS
        # Each category becomes a gallery with materials as items
        for category_name, materials in ACOUSTIC_MATERIALS.items():
            # Convert materials to gallery items format: (name, color, absorption)
            material_items = [
                (material_name, color, absorption_coeff)
                for material_name, absorption_coeff, color in materials
            ]
            
            # Create gallery at initial position (will be repositioned dynamically)
            gallery = MaterialGallery(
                self.rect.x + 5, self.content_y + 5,
                self.rect.width - 10, category_name, material_items,
                callback=self.on_material_select
            )
            gallery.enabled = True  # Enable material selection
            self.material_galleries.append(gallery)
        
        # Initial positioning
        self._reposition_galleries()
    
    def on_material_select(self, material_name: str, color: Tuple[int, int, int], absorption: float):
        """Handle material selection - apply to selected surface"""
        if self.renderer and self.renderer.selected_surface_index is not None:
            # Convert color from 0-255 to 0-1 range for OpenGL
            gl_color = (color[0] / 255.0, color[1] / 255.0, color[2] / 255.0)
            self.renderer.set_surface_color(self.renderer.selected_surface_index, gl_color)
            print(f"Applied material '{material_name}' (absorption: {absorption}) to surface {self.renderer.selected_surface_index + 1}")
        else:
            print(f"Select a surface first to apply material '{material_name}'")
    
    def _reposition_galleries(self):
        """Reposition galleries to stack towards the top based on their collapsed state"""
        galleries = self.sound_galleries if self.active_tab == "SOUND" else self.material_galleries
        
        current_y = self.content_y + 5
        for gallery in galleries:
            old_y = gallery.rect.y
            gallery.rect.y = current_y
            
            # Update item positions when gallery moves (handle both types)
            y_offset = current_y - old_y
            items = getattr(gallery, 'image_items', None) or getattr(gallery, 'material_items', [])
            for item in items:
                item.rect.y += y_offset
            
            current_y += gallery.rect.height + 5
    
    def set_renderer(self, renderer):
        """Set the renderer reference for material application"""
        self.renderer = renderer
    
    def handle_event(self, event: pygame.event.Event) -> bool:
        if not self.visible or not self.enabled:
            return False
        
        # Handle tab clicks
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            tab_y = self.rect.y + self.header_height
            sound_tab_rect = pygame.Rect(self.rect.x + 5, tab_y, self.tab_width, self.tab_height)
            material_tab_rect = pygame.Rect(self.rect.x + 5 + self.tab_width, tab_y, self.tab_width, self.tab_height)
            
            if sound_tab_rect.collidepoint(event.pos):
                self.active_tab = "SOUND"
                self._reposition_galleries()
                return True
            elif material_tab_rect.collidepoint(event.pos):
                self.active_tab = "MATERIAL"
                self._reposition_galleries()
                return True
        
        # Handle gallery events based on active tab
        galleries = self.sound_galleries if self.active_tab == "SOUND" else self.material_galleries
        gallery_changed = False
        for gallery in galleries:
            if gallery.handle_event(event):
                gallery_changed = True
        
        # Reposition galleries if any gallery was collapsed/expanded
        if gallery_changed:
            self._reposition_galleries()
            return True
        
        return False
    
    def update(self, dt: float):
        galleries = self.sound_galleries if self.active_tab == "SOUND" else self.material_galleries
        for gallery in galleries:
            gallery.update(dt)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Draw main background
        pygame.draw.rect(surface, self.bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 2)
        
        # Draw header
        header_rect = pygame.Rect(self.rect.x, self.rect.y, self.rect.width, self.header_height)
        pygame.draw.rect(surface, self.header_bg_color, header_rect)
        pygame.draw.rect(surface, self.border_color, header_rect, 1)
        
        # Draw header text
        header_text = self.font.render("LIBRARY", True, self.header_text_color)
        header_text_rect = header_text.get_rect(center=header_rect.center)
        surface.blit(header_text, header_text_rect)
        
        # Draw tabs
        tab_y = self.rect.y + self.header_height
        sound_tab_rect = pygame.Rect(self.rect.x + 5, tab_y, self.tab_width, self.tab_height)
        material_tab_rect = pygame.Rect(self.rect.x + 5 + self.tab_width, tab_y, self.tab_width, self.tab_height)
        
        # Sound tab
        sound_bg = self.tab_active_color if self.active_tab == "SOUND" else self.tab_inactive_color
        pygame.draw.rect(surface, sound_bg, sound_tab_rect)
        pygame.draw.rect(surface, self.border_color, sound_tab_rect, 1)
        sound_text = self.tab_font.render("SOUND", True, self.tab_text_color)
        sound_text_rect = sound_text.get_rect(center=sound_tab_rect.center)
        surface.blit(sound_text, sound_text_rect)
        
        # Material tab
        material_bg = self.tab_active_color if self.active_tab == "MATERIAL" else self.tab_inactive_color
        pygame.draw.rect(surface, material_bg, material_tab_rect)
        pygame.draw.rect(surface, self.border_color, material_tab_rect, 1)
        material_text = self.tab_font.render("MATERIAL", True, self.tab_text_color)
        material_text_rect = material_text.get_rect(center=material_tab_rect.center)
        surface.blit(material_text, material_text_rect)
        
        # Draw active tab content
        content_rect = pygame.Rect(self.rect.x + 1, self.content_y + 1,
                                 self.rect.width - 2, self.content_height - 2)
        clip_rect = surface.get_clip()
        surface.set_clip(content_rect)
        
        galleries = self.sound_galleries if self.active_tab == "SOUND" else self.material_galleries
        for gallery in galleries:
            gallery.draw(surface)
        
        surface.set_clip(clip_rect)


class PropertyPanel(GUIComponent):
    """A specialized panel for properties with various GUI elements"""
    
    def __init__(self, x: int, y: int, width: int, height: int):
        super().__init__(x, y, width, height)
        self.font = pygame.font.Font(None, 16)
        self.label_font = pygame.font.Font(None, 14)
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.header_bg_color = Colors.LIGHT_GRAY
        self.header_text_color = Colors.BLACK
        
        # Layout
        self.header_height = 25
        self.content_y = self.rect.y + self.header_height
        self.content_height = self.rect.height - self.header_height
        
        # Reference to the renderer (set by application)
        self.renderer = None
        
        # Create GUI elements
        self._create_elements()
    
    def _create_elements(self):
        """Create all the property elements"""
        current_y = self.content_y + 10
        
        # Room Scale Slider
        self.scale_label = TextButton(self.rect.x + 10, current_y, 100, 20, "Room Scale", 14)
        current_y += 25
        self.scale_value_label = TextButton(self.rect.x + self.rect.width - 50, current_y - 25, 45, 20, "1.0x", 14)
        self.scale_slider = Slider(self.rect.x + 10, current_y, self.rect.width - 60, 20, 0.1, 10.0, 1.0, self.on_scale_change)
        current_y += 25
        
        # Dimension display label
        self.dimension_label = TextButton(self.rect.x + 10, current_y, self.rect.width - 20, 20, "Size: -- m", 12)
        current_y += 35
        
        # Point Distance Slider (for 3D point placement)
        self.point_distance_label = TextButton(self.rect.x + 10, current_y, 100, 20, "Point Distance", 14)
        current_y += 25
        self.point_distance_value_label = TextButton(self.rect.x + self.rect.width - 55, current_y - 25, 50, 20, "0.0m", 14)
        self.point_distance_slider = Slider(self.rect.x + 10, current_y, self.rect.width - 60, 20, -5.0, 5.0, 0.0, self.on_point_distance_change)
        current_y += 25
        
        # ============ Selected Point Section ============
        # Point type label
        self.point_type_label = TextButton(self.rect.x + 10, current_y, 60, 18, "Type:", 11)
        current_y += 20
        
        # Point type buttons (Source / Listener)
        button_width = 70
        self.point_type_source_btn = TextButton(self.rect.x + 10, current_y, button_width, 24, "Source", 11,
                                                callback=self.on_set_point_source)
        self.point_type_source_btn.enabled = False
        
        self.point_type_listener_btn = TextButton(self.rect.x + 15 + button_width, current_y, button_width, 24, "Listener", 11,
                                                  callback=self.on_set_point_listener)
        self.point_type_listener_btn.enabled = False
        current_y += 28
        
        # Delete Point button
        self.delete_point_button = TextButton(self.rect.x + 10, current_y, 80, 24, "Delete", 12, 
                                              callback=self.on_delete_point)
        self.delete_point_button.enabled = False
        
        # Deselect Point button
        self.deselect_point_button = TextButton(self.rect.x + 95, current_y, 80, 24, "Deselect", 12,
                                          callback=self.on_deselect_point)
        self.deselect_point_button.enabled = False
        current_y += 28
        
        # ============ Selected Surface Section ============
        # # Surface area label
        # self.surface_area_label = TextButton(self.rect.x + 10, current_y, self.rect.width - 20, 16, "Area: -- m²", 11)
        # current_y += 20
        
        # Toggle Texture button
        self.toggle_texture_button = TextButton(self.rect.x + 10, current_y, 80, 24, "Texture", 12,
                                                callback=self.on_toggle_texture)
        self.toggle_texture_button.enabled = False
        
        # Deselect Surface button
        self.deselect_surface_button = TextButton(self.rect.x + 95, current_y, 80, 24, "Deselect", 12,
                                                  callback=self.on_deselect_surface)
        self.deselect_surface_button.enabled = False
        
        # Store all components for easy handling
        self.components = [
            self.scale_label, self.scale_value_label, self.scale_slider, self.dimension_label,
            self.point_distance_label, self.point_distance_value_label, self.point_distance_slider,
            self.point_type_label, self.point_type_source_btn, self.point_type_listener_btn,
            self.delete_point_button, self.deselect_point_button,
            self.toggle_texture_button, self.deselect_surface_button
        ]
    
    def on_scale_change(self, value):
        """Handle scale slider changes"""
        self.scale_value_label.text = f"{value:.1f}x"
        
        # Update renderer if available
        if self.renderer:
            self.renderer.set_scale_factor(value)
            # Update dimension display
            dimensions = self.renderer.get_real_world_dimensions()
            dim_text = f"Size: {dimensions[0]:.1f}x{dimensions[1]:.1f}x{dimensions[2]:.1f}m"
            self.dimension_label.text = dim_text
        
        print(f"Scale factor: {value:.2f}x")
    
    def on_point_distance_change(self, value):
        """Handle point distance slider changes"""
        self.point_distance_value_label.text = f"{value:.1f}m"
        
        # Update renderer's active point distance
        if self.renderer:
            self.renderer.update_active_point_distance(value)
    
    def on_delete_point(self):
        """Delete the currently selected point"""
        if self.renderer and self.renderer.active_point_index is not None:
            self.renderer.remove_active_point()
    
    def on_deselect_point(self):
        """Deselect the currently selected point"""
        if self.renderer:
            self.renderer.deselect_point()
    
    def on_set_point_source(self):
        """Set the selected point as a sound source"""
        if self.renderer and self.renderer.active_point_index is not None:
            self.renderer.set_active_point_type("source")
    
    def on_set_point_listener(self):
        """Set the selected point as a listener"""
        if self.renderer and self.renderer.active_point_index is not None:
            self.renderer.set_active_point_type("listener")
    
    def on_toggle_texture(self):
        """Toggle texture on the selected surface"""
        if self.renderer and self.renderer.selected_surface_index is not None:
            self.renderer.toggle_surface_texture(self.renderer.selected_surface_index)
    
    def on_deselect_surface(self):
        """Deselect the currently selected surface"""
        if self.renderer:
            self.renderer.deselect_surface()
    
    def set_renderer(self, renderer):
        """Set the renderer reference for scale control"""
        self.renderer = renderer
        if renderer:
            # Update slider to reflect current scale
            self.scale_slider.value = renderer.model_scale_factor
            self.scale_value_label.text = f"{renderer.model_scale_factor:.1f}x"
            # Update dimension display
            dimensions = renderer.get_real_world_dimensions()
            dim_text = f"Size: {dimensions[0]:.1f}x{dimensions[1]:.1f}x{dimensions[2]:.1f}m"
            self.dimension_label.text = dim_text
    
    def handle_event(self, event: pygame.event.Event) -> bool:
        if not self.visible or not self.enabled:
            return False
        
        for component in self.components:
            if component.handle_event(event):
                return True
        return False
    
    def update(self, dt: float):
        for component in self.components:
            component.update(dt)
        
        # Sync with renderer state
        if self.renderer:
            # Check if a point is selected
            if self.renderer.active_point_index is not None:
                point_idx = self.renderer.active_point_index
                point = self.renderer.placed_points[point_idx]
                
                # Enable point action buttons
                self.delete_point_button.enabled = True
                self.deselect_point_button.enabled = True
                
                # Enable and update point type buttons
                self.point_type_source_btn.enabled = True
                self.point_type_listener_btn.enabled = True
                
                # Highlight the active type button
                current_type = point.point_type
                if current_type == "source":
                    self.point_type_source_btn.text = "● Source"
                    self.point_type_listener_btn.text = "Listener"
                elif current_type == "listener":
                    self.point_type_source_btn.text = "Source"
                    self.point_type_listener_btn.text = "● Listener"
                else:
                    self.point_type_source_btn.text = "Source"
                    self.point_type_listener_btn.text = "Listener"
                
                # Sync distance slider with active point's distance
                current_distance = point.distance
                if abs(self.point_distance_slider.value - current_distance) > 0.01:
                    self.point_distance_slider.value = current_distance
                    self.point_distance_value_label.text = f"{current_distance:.1f}m"
            else:
                
                # Disable point action buttons
                self.delete_point_button.enabled = False
                self.deselect_point_button.enabled = False
                
                # Disable and reset type buttons
                self.point_type_source_btn.enabled = False
                self.point_type_listener_btn.enabled = False
                self.point_type_source_btn.text = "Source"
                self.point_type_listener_btn.text = "Listener"
            
            # Check if a surface is selected
            if self.renderer.selected_surface_index is not None:
                surface_info = self.renderer.get_selected_surface_info()
                if surface_info:
                    # Update surface info labels
                    #self.surface_area_label.text = f"Area: {surface_info['area']:.2f} m²"
                    
                    # Update texture button text based on current state
                    if surface_info['has_texture']:
                        self.toggle_texture_button.text = "No Texture"
                    else:
                        self.toggle_texture_button.text = "Texture"
                    
                    # Enable surface action buttons
                    self.toggle_texture_button.enabled = True
                    self.deselect_surface_button.enabled = True
            else:
                # No surface selected - show placeholder values
                #self.surface_area_label.text = "Area: -- m²"
                self.toggle_texture_button.text = "Texture"
                
                # Disable surface action buttons
                self.toggle_texture_button.enabled = False
                self.deselect_surface_button.enabled = False
    
    def draw(self, surface: pygame.Surface):
        """Draw both the panel and dropdowns"""
        self.draw_base(surface)
        self.draw_dropdowns(surface)
    
    def draw_base(self, surface: pygame.Surface):
        """Draw the panel without dropdowns"""
        if not self.visible:
            return
        
        # Draw main background
        pygame.draw.rect(surface, self.bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 2)
        
        # Draw header
        header_rect = pygame.Rect(self.rect.x, self.rect.y, self.rect.width, self.header_height)
        pygame.draw.rect(surface, self.header_bg_color, header_rect)
        pygame.draw.rect(surface, self.border_color, header_rect, 1)
        
        # Draw header text
        header_text = self.font.render("PROPERTY", True, self.header_text_color)
        header_text_rect = header_text.get_rect(center=header_rect.center)
        surface.blit(header_text, header_text_rect)
        
        # Draw components with clipping (excluding dropdown menus)
        content_rect = pygame.Rect(self.rect.x + 1, self.content_y + 1,
                                 self.rect.width - 2, self.content_height - 2)
        clip_rect = surface.get_clip()
        surface.set_clip(content_rect)
        
        for component in self.components:
            if isinstance(component, DropdownMenu):
                # Draw dropdown base but not its expanded items
                component.draw_base(surface)
            else:
                component.draw(surface)
        
        surface.set_clip(clip_rect)
    
    def draw_dropdowns(self, surface: pygame.Surface):
        """Draw only the dropdown expanded items (on top of everything)"""
        if not self.visible:
            return
        
        for component in self.components:
            if isinstance(component, DropdownMenu):
                component.draw_dropdowns(surface)


class AssetsPanel(GUIComponent):
    """A specialized panel for assets with dropdown gallery"""
    
    def __init__(self, x: int, y: int, width: int, height: int):
        super().__init__(x, y, width, height)
        self.font = pygame.font.Font(None, 16)
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.header_bg_color = Colors.LIGHT_GRAY
        self.header_text_color = Colors.BLACK
        
        # Layout
        self.header_height = 25
        self.content_y = self.rect.y + self.header_height
        self.content_height = self.rect.height - self.header_height
        
        # Create asset galleries
        self._create_galleries()
    
    def _create_galleries(self):
        """Create asset galleries - starts empty"""
        self.galleries = []
    
    def add_stl_surfaces(self, stl_filename: str, surfaces: List[Tuple[str, Tuple[int, int, int], int]]):
        """Add surfaces from an STL file to the assets panel"""
        print(f"AssetsPanel.add_stl_surfaces called with {len(surfaces)} surfaces")
        
        # Clear existing galleries
        self.galleries.clear()
        print("Cleared existing galleries")
        
        # Extract just the filename without path and extension
        stl_name = os.path.splitext(os.path.basename(stl_filename))[0]
        print(f"STL name: {stl_name}")
        
        if not surfaces:
            print("WARNING: No surfaces provided!")
            return
        
        # Create a surface gallery for this STL file
        print(f"Creating SurfaceGallery at ({self.rect.x + 5}, {self.content_y + 5}) with width {self.rect.width - 10}")
        
        try:
            surface_gallery = SurfaceGallery(
                self.rect.x + 5, self.content_y + 5, 
                self.rect.width - 10, stl_name, 
                surfaces, 250, self.on_surface_select
            )
            
            self.galleries = [surface_gallery]
            print(f"Created gallery with {len(surface_gallery.surface_items)} surface items")
            
            self._reposition_galleries()
            print("Repositioned galleries")
            
        except Exception as e:
            print(f"Error creating SurfaceGallery: {e}")
            import traceback
            traceback.print_exc()
    
    def update_surface_color(self, surface_index: int, new_color: Tuple[int, int, int]):
        """Update the color of a surface in the assets panel"""
        for gallery in self.galleries:
            if isinstance(gallery, SurfaceGallery):
                gallery.update_surface_color(surface_index, new_color)
    
    def clear_surfaces(self):
        """Clear all surface galleries"""
        self.galleries.clear()
    
    def on_surface_select(self, surface_index: int, surface_name: str):
        """Handle surface selection from the assets panel"""
        print(f"Selected surface {surface_index}: {surface_name}")
        # You could add functionality here to highlight the surface in the 3D view
    
    def on_asset_select(self, asset):
        print(f"Asset selected: {asset}")
    
    def handle_event(self, event: pygame.event.Event) -> bool:
        if not self.visible or not self.enabled:
            return False
        
        gallery_changed = False
        for gallery in self.galleries:
            if gallery.handle_event(event):
                gallery_changed = True
        
        # Reposition if needed (similar to library panel)
        if gallery_changed:
            self._reposition_galleries()
            return True
        
        return False
    
    def _reposition_galleries(self):
        """Reposition galleries to stack towards the top"""
        current_y = self.content_y + 5
        for gallery in self.galleries:
            old_y = gallery.rect.y
            gallery.rect.y = current_y
            
            # Update item positions (handle both SurfaceGallery and ImageGallery)
            y_offset = current_y - old_y
            if hasattr(gallery, 'surface_items'):
                items = gallery.surface_items
            elif hasattr(gallery, 'image_items'):
                items = gallery.image_items
            else:
                items = []
            
            for item in items:
                item.rect.y += y_offset
            
            current_y += gallery.rect.height + 5
    
    def update(self, dt: float):
        for gallery in self.galleries:
            gallery.update(dt)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Draw main background
        pygame.draw.rect(surface, self.bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 2)
        
        # Draw header
        header_rect = pygame.Rect(self.rect.x, self.rect.y, self.rect.width, self.header_height)
        pygame.draw.rect(surface, self.header_bg_color, header_rect)
        pygame.draw.rect(surface, self.border_color, header_rect, 1)
        
        # Draw header text
        header_text = self.font.render("ASSETS", True, self.header_text_color)
        header_text_rect = header_text.get_rect(center=header_rect.center)
        surface.blit(header_text, header_text_rect)
        
        # Draw galleries with clipping
        content_rect = pygame.Rect(self.rect.x + 1, self.content_y + 1,
                                 self.rect.width - 2, self.content_height - 2)
        clip_rect = surface.get_clip()
        surface.set_clip(content_rect)
        
        for gallery in self.galleries:
            gallery.draw(surface)
        
        surface.set_clip(clip_rect)
