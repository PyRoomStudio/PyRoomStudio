import pygame
import pygame.font
import pygame.gfxdraw
import math
from typing import Tuple, List, Callable, Optional, Any
from enum import Enum
from OpenGL.GL import *
from OpenGL.GLU import *
import os

# Initialize pygame
pygame.init()

# Color constants
class Colors:
    WHITE = (255, 255, 255)
    BLACK = (0, 0, 0)
    GRAY = (128, 128, 128)
    LIGHT_GRAY = (200, 200, 200)
    DARK_GRAY = (64, 64, 64)
    BLUE = (70, 130, 180)
    LIGHT_BLUE = (173, 216, 230)
    GREEN = (60, 179, 113)
    RED = (220, 20, 60)
    ORANGE = (255, 140, 0)
    YELLOW = (255, 255, 0)

class GUIComponent:
    """Base class for all GUI components"""
    
    def __init__(self, x: int, y: int, width: int, height: int):
        self.rect = pygame.Rect(x, y, width, height)
        self.visible = True
        self.enabled = True
        self.hover = False
        self.clicked = False
        
    def handle_event(self, event: pygame.event.Event) -> bool:
        """Handle pygame events. Returns True if event was consumed."""
        if not self.visible or not self.enabled:
            return False
            
        if event.type == pygame.MOUSEMOTION:
            self.hover = self.rect.collidepoint(event.pos)
            
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1 and self.rect.collidepoint(event.pos):
                self.clicked = True
                return True
                
        elif event.type == pygame.MOUSEBUTTONUP:
            if event.button == 1 and self.clicked:
                self.clicked = False
                if self.rect.collidepoint(event.pos):
                    self.on_click()
                    return True
                    
        return False
    
    def on_click(self):
        """Override in subclasses to handle click events"""
        pass
    
    def update(self, dt: float):
        """Update component state. Override in subclasses if needed."""
        pass
    
    def draw(self, surface: pygame.Surface):
        """Draw the component. Must be implemented by subclasses."""
        raise NotImplementedError

class TextButton(GUIComponent):
    """A clickable text button with hover effects"""
    
    def __init__(self, x: int, y: int, width: int, height: int, text: str, 
                 font_size: int = 16, callback: Optional[Callable] = None):
        super().__init__(x, y, width, height)
        self.text = text
        self.callback = callback
        self.font = pygame.font.Font(None, font_size)
        
        # Colors
        self.bg_color = Colors.LIGHT_GRAY
        self.hover_color = Colors.BLUE
        self.text_color = Colors.BLACK
        self.border_color = Colors.DARK_GRAY
        
    def on_click(self):
        if self.callback:
            self.callback()
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
            
        # Choose background color based on state
        bg_color = self.hover_color if self.hover else self.bg_color
        if self.clicked:
            bg_color = tuple(max(0, c - 30) for c in bg_color)
        
        # Draw button background
        pygame.draw.rect(surface, bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 2)
        
        # Draw text
        text_surface = self.font.render(self.text, True, self.text_color)
        text_rect = text_surface.get_rect(center=self.rect.center)
        surface.blit(text_surface, text_rect)

class ImageButton(GUIComponent):
    """A clickable button with an image"""
    
    def __init__(self, x: int, y: int, width: int, height: int, 
                 image_path: str, callback: Optional[Callable] = None):
        super().__init__(x, y, width, height)
        self.callback = callback
        
        try:
            self.image = pygame.image.load(image_path)
            self.image = pygame.transform.scale(self.image, (width - 4, height - 4))
        except pygame.error:
            # Create a placeholder if image can't be loaded
            self.image = pygame.Surface((width - 4, height - 4))
            self.image.fill(Colors.GRAY)
        
        self.border_color = Colors.DARK_GRAY
        self.hover_color = Colors.LIGHT_BLUE
    
    def on_click(self):
        if self.callback:
            self.callback()
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
            
        # Draw background with hover effect
        if self.hover:
            pygame.draw.rect(surface, self.hover_color, self.rect)
        
        # Draw border
        pygame.draw.rect(surface, self.border_color, self.rect, 2)
        
        # Draw image
        image_rect = self.image.get_rect(center=self.rect.center)
        surface.blit(self.image, image_rect)

class DropdownMenu(GUIComponent):
    """A dropdown menu for selecting from a list of options"""
    
    def __init__(self, x: int, y: int, width: int, height: int, 
                 options: List[str], font_size: int = 16, 
                 callback: Optional[Callable[[str], None]] = None):
        super().__init__(x, y, width, height)
        self.options = options
        self.selected_index = 0
        self.expanded = False
        self.callback = callback
        self.font = pygame.font.Font(None, font_size)
        
        # Calculate expanded height
        self.item_height = height
        self.expanded_height = self.item_height * len(options)
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.hover_color = Colors.LIGHT_BLUE
        self.text_color = Colors.BLACK
    
    def handle_event(self, event: pygame.event.Event) -> bool:
        if not self.visible or not self.enabled:
            return False
        
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            if self.rect.collidepoint(event.pos):
                self.expanded = not self.expanded
                return True
            elif self.expanded:
                # Check if clicking on an expanded option
                expanded_rect = pygame.Rect(self.rect.x, self.rect.y, 
                                          self.rect.width, self.expanded_height)
                if expanded_rect.collidepoint(event.pos):
                    # Calculate which option was clicked
                    relative_y = event.pos[1] - self.rect.y
                    clicked_index = relative_y // self.item_height
                    if 0 <= clicked_index < len(self.options):
                        self.selected_index = clicked_index
                        self.expanded = False
                        if self.callback:
                            self.callback(self.options[self.selected_index])
                        return True
                else:
                    self.expanded = False
        
        return False
    
    def draw(self, surface: pygame.Surface):
        """Draw both the dropdown button and expanded options"""
        self.draw_base(surface)
        self.draw_dropdowns(surface)
    
    def draw_base(self, surface: pygame.Surface):
        """Draw just the dropdown button"""
        if not self.visible:
            return
        
        # Draw main button
        pygame.draw.rect(surface, self.bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 2)
        
        # Draw selected text
        if self.options:
            text = self.options[self.selected_index]
            text_surface = self.font.render(text, True, self.text_color)
            text_rect = text_surface.get_rect(midleft=(self.rect.x + 5, self.rect.centery))
            surface.blit(text_surface, text_rect)
        
        # Draw dropdown arrow
        arrow_points = [
            (self.rect.right - 15, self.rect.centery - 3),
            (self.rect.right - 5, self.rect.centery - 3),
            (self.rect.right - 10, self.rect.centery + 3)
        ]
        pygame.draw.polygon(surface, self.text_color, arrow_points)
    
    def draw_dropdowns(self, surface: pygame.Surface):
        """Draw only the expanded dropdown options (on top of everything)"""
        if not self.visible or not self.expanded:
            return
        
        # Draw expanded options
        for i, option in enumerate(self.options):
            option_rect = pygame.Rect(self.rect.x, self.rect.y + i * self.item_height,
                                    self.rect.width, self.item_height)
            
            # Highlight hovered option
            mouse_pos = pygame.mouse.get_pos()
            if option_rect.collidepoint(mouse_pos):
                pygame.draw.rect(surface, self.hover_color, option_rect)
            else:
                pygame.draw.rect(surface, self.bg_color, option_rect)
            
            pygame.draw.rect(surface, self.border_color, option_rect, 1)
            
            # Draw option text
            text_surface = self.font.render(option, True, self.text_color)
            text_rect = text_surface.get_rect(midleft=(option_rect.x + 5, option_rect.centery))
            surface.blit(text_surface, text_rect)

class Slider(GUIComponent):
    """A slider for selecting numeric values"""
    
    def __init__(self, x: int, y: int, width: int, height: int,
                 min_value: float = 0.0, max_value: float = 1.0, 
                 initial_value: float = 0.5, callback: Optional[Callable[[float], None]] = None):
        super().__init__(x, y, width, height)
        self.min_value = min_value
        self.max_value = max_value
        self.value = initial_value
        self.callback = callback
        self.dragging = False
        
        # Colors
        self.track_color = Colors.LIGHT_GRAY
        self.handle_color = Colors.BLUE
        self.handle_hover_color = Colors.LIGHT_BLUE
        
        # Handle dimensions
        self.handle_width = 20
        self.handle_height = height
    
    def handle_event(self, event: pygame.event.Event) -> bool:
        if not self.visible or not self.enabled:
            return False
        
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            handle_x = self._get_handle_x()
            handle_rect = pygame.Rect(handle_x, self.rect.y, 
                                    self.handle_width, self.handle_height)
            if handle_rect.collidepoint(event.pos):
                self.dragging = True
                return True
        
        elif event.type == pygame.MOUSEBUTTONUP and event.button == 1:
            self.dragging = False
        
        elif event.type == pygame.MOUSEMOTION and self.dragging:
            # Update value based on mouse position
            relative_x = event.pos[0] - self.rect.x - self.handle_width // 2
            track_width = self.rect.width - self.handle_width
            relative_x = max(0, min(track_width, relative_x))
            
            self.value = self.min_value + (relative_x / track_width) * (self.max_value - self.min_value)
            
            if self.callback:
                self.callback(self.value)
            return True
        
        elif event.type == pygame.MOUSEMOTION:
            handle_x = self._get_handle_x()
            handle_rect = pygame.Rect(handle_x, self.rect.y, 
                                    self.handle_width, self.handle_height)
            self.hover = handle_rect.collidepoint(event.pos)
        
        return False
    
    def _get_handle_x(self) -> int:
        """Get the x position of the handle based on current value"""
        track_width = self.rect.width - self.handle_width
        relative_value = (self.value - self.min_value) / (self.max_value - self.min_value)
        return self.rect.x + int(relative_value * track_width)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Draw track
        track_rect = pygame.Rect(self.rect.x, self.rect.centery - 2, 
                               self.rect.width, 4)
        pygame.draw.rect(surface, self.track_color, track_rect)
        pygame.draw.rect(surface, Colors.DARK_GRAY, track_rect, 1)
        
        # Draw handle
        handle_x = self._get_handle_x()
        handle_rect = pygame.Rect(handle_x, self.rect.y, 
                                self.handle_width, self.handle_height)
        
        handle_color = self.handle_hover_color if (self.hover or self.dragging) else self.handle_color
        pygame.draw.rect(surface, handle_color, handle_rect)
        pygame.draw.rect(surface, Colors.DARK_GRAY, handle_rect, 2)

class ToggleButton(GUIComponent):
    """A toggle button that can be in on/off state"""
    
    def __init__(self, x: int, y: int, width: int, height: int, 
                 text: str = "", initial_state: bool = False, 
                 font_size: int = 16, callback: Optional[Callable[[bool], None]] = None):
        super().__init__(x, y, width, height)
        self.text = text
        self.state = initial_state
        self.callback = callback
        self.font = pygame.font.Font(None, font_size)
        
        # Colors
        self.on_color = Colors.GREEN
        self.off_color = Colors.LIGHT_GRAY
        self.text_color = Colors.BLACK
        self.border_color = Colors.DARK_GRAY
    
    def on_click(self):
        self.state = not self.state
        if self.callback:
            self.callback(self.state)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Choose colors based on state
        bg_color = self.on_color if self.state else self.off_color
        if self.hover:
            bg_color = tuple(min(255, c + 20) for c in bg_color)
        if self.clicked:
            bg_color = tuple(max(0, c - 30) for c in bg_color)
        
        # Draw button
        pygame.draw.rect(surface, bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 2)
        
        # Draw text
        if self.text:
            text_surface = self.font.render(self.text, True, self.text_color)
            text_rect = text_surface.get_rect(center=self.rect.center)
            surface.blit(text_surface, text_rect)
        
        # Draw state indicator
        indicator_size = min(self.rect.width // 4, self.rect.height // 4)
        indicator_x = self.rect.right - indicator_size - 5
        indicator_y = self.rect.y + 5
        
        indicator_color = Colors.WHITE if self.state else Colors.DARK_GRAY
        pygame.draw.circle(surface, indicator_color, 
                         (indicator_x + indicator_size // 2, indicator_y + indicator_size // 2), 
                         indicator_size // 2)

class MenuItem(GUIComponent):
    """A single item in a dropdown menu"""
    
    def __init__(self, x: int, y: int, width: int, height: int, text: str,
                 callback: Optional[Callable] = None, font_size: int = 14):
        super().__init__(x, y, width, height)
        self.text = text
        self.callback = callback
        self.font = pygame.font.Font(None, font_size)
        
        # Colors
        self.bg_color = Colors.WHITE
        self.hover_color = Colors.LIGHT_BLUE
        self.text_color = Colors.BLACK
        self.border_color = Colors.LIGHT_GRAY
    
    def on_click(self):
        if self.callback:
            self.callback()
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Background
        bg_color = self.hover_color if self.hover else self.bg_color
        pygame.draw.rect(surface, bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 1)
        
        # Text
        text_surface = self.font.render(self.text, True, self.text_color)
        text_rect = text_surface.get_rect(midleft=(self.rect.x + 8, self.rect.centery))
        surface.blit(text_surface, text_rect)

class MenuButton(GUIComponent):
    """A menu button that doesn't draw borders"""
    
    def __init__(self, x: int, y: int, width: int, height: int, text: str, 
                 font_size: int = 14, callback: Optional[Callable] = None):
        super().__init__(x, y, width, height)
        self.text = text
        self.callback = callback
        self.font = pygame.font.Font(None, font_size)
    
    def on_click(self):
        if self.callback:
            self.callback()

class MenuBar(GUIComponent):
    """A menu bar with dropdown menus"""
    
    def __init__(self, x: int, y: int, width: int, height: int = 25):
        super().__init__(x, y, width, height)
        self.menus: List[Tuple[str, List[Tuple[str, Callable]]]] = []
        self.menu_buttons: List[MenuButton] = []
        self.active_menu_index = -1
        self.dropdown_items: List[MenuItem] = []
        self.font = pygame.font.Font(None, 16)
        
        # Colors
        self.bg_color = Colors.LIGHT_GRAY
        self.border_color = Colors.DARK_GRAY
        
        # Menu dimensions
        self.menu_item_height = 25
        self.dropdown_width = 150
    
    def add_menu(self, title: str, items: List[Tuple[str, Callable]]):
        """Add a menu with title and list of (item_name, callback) tuples"""
        self.menus.append((title, items))
        self._rebuild_menu_buttons()
    
    def _rebuild_menu_buttons(self):
        """Rebuild menu buttons after adding menus"""
        self.menu_buttons.clear()
        x_offset = self.rect.x + 5
        
        for i, (title, _) in enumerate(self.menus):
            button_width = len(title) * 8 + 16  # Approximate width based on text
            button = MenuButton(x_offset, self.rect.y, button_width, self.rect.height, 
                              title, 14, lambda idx=i: self._toggle_menu(idx))
            self.menu_buttons.append(button)
            x_offset += button_width
    
    def _toggle_menu(self, menu_index: int):
        """Toggle dropdown menu visibility"""
        if self.active_menu_index == menu_index:
            self.active_menu_index = -1
            self.dropdown_items.clear()
        else:
            self.active_menu_index = menu_index
            self._create_dropdown_items(menu_index)
    
    def _create_dropdown_items(self, menu_index: int):
        """Create dropdown menu items for the specified menu"""
        self.dropdown_items.clear()
        
        if 0 <= menu_index < len(self.menus):
            _, items = self.menus[menu_index]
            button = self.menu_buttons[menu_index]
            
            start_y = self.rect.bottom
            
            for i, (item_text, callback) in enumerate(items):
                item_y = start_y + i * self.menu_item_height
                
                def item_callback(cb=callback):
                    cb()
                    self.active_menu_index = -1  # Close menu after selection
                    self.dropdown_items.clear()
                
                menu_item = MenuItem(button.rect.x, item_y, self.dropdown_width, 
                                   self.menu_item_height, item_text, item_callback)
                self.dropdown_items.append(menu_item)
    
    def handle_event(self, event: pygame.event.Event) -> bool:
        if not self.visible or not self.enabled:
            return False
        
        # Handle dropdown item events first
        for item in self.dropdown_items:
            if item.handle_event(event):
                return True
        
        # Handle menu button events
        for button in self.menu_buttons:
            if button.handle_event(event):
                return True
        
        # Close dropdown if clicking outside
        if event.type == pygame.MOUSEBUTTONDOWN and self.active_menu_index != -1:
            # Check if click is outside the menu area
            menu_area = pygame.Rect(self.rect.x, self.rect.y, 
                                  self.dropdown_width, 
                                  self.rect.height + len(self.dropdown_items) * self.menu_item_height)
            if not menu_area.collidepoint(event.pos):
                self.active_menu_index = -1
                self.dropdown_items.clear()
                return True
        
        return False
    
    def update(self, dt: float):
        for button in self.menu_buttons:
            button.update(dt)
        for item in self.dropdown_items:
            item.update(dt)
    
    def draw(self, surface: pygame.Surface):
        """Draw both the menu bar and dropdowns"""
        self.draw_base(surface)
        self.draw_dropdowns(surface)
    
    def draw_base(self, surface: pygame.Surface):
        """Draw just the menu bar without dropdowns"""
        if not self.visible:
            return
        
        # Draw menu bar background
        pygame.draw.rect(surface, self.bg_color, self.rect)
        # Draw only bottom border line
        pygame.draw.line(surface, self.border_color, 
                        (self.rect.x, self.rect.bottom - 1), 
                        (self.rect.right, self.rect.bottom - 1))
        
        # Draw menu buttons without individual borders
        for i, button in enumerate(self.menu_buttons):
            # Highlight active menu button
            if i == self.active_menu_index:
                bg_color = Colors.BLUE
                text_color = Colors.WHITE
            elif button.hover:
                bg_color = Colors.BLUE
                text_color = Colors.WHITE
            else:
                bg_color = Colors.LIGHT_GRAY
                text_color = Colors.BLACK
            
            # Draw button background without border
            pygame.draw.rect(surface, bg_color, button.rect)
            
            # Draw text
            text_surface = button.font.render(button.text, True, text_color)
            text_rect = text_surface.get_rect(center=button.rect.center)
            surface.blit(text_surface, text_rect)
    
    def draw_dropdowns(self, surface: pygame.Surface):
        """Draw only the dropdown menus (on top of everything)"""
        if not self.visible or not self.dropdown_items:
            return
        
        # Draw dropdown background
        dropdown_rect = pygame.Rect(
            self.dropdown_items[0].rect.x - 1,
            self.dropdown_items[0].rect.y - 1,
            self.dropdown_width + 2,
            len(self.dropdown_items) * self.menu_item_height + 2
        )
        pygame.draw.rect(surface, Colors.WHITE, dropdown_rect)
        pygame.draw.rect(surface, Colors.DARK_GRAY, dropdown_rect, 2)
        
        for item in self.dropdown_items:
            item.draw(surface)

class ImageItem(GUIComponent):
    """An image item with a label for galleries"""
    
    def __init__(self, x: int, y: int, width: int, height: int, 
                 image_path: str, label: str, callback: Optional[Callable] = None):
        super().__init__(x, y, width, height)
        self.label = label
        self.callback = callback
        self.font = pygame.font.Font(None, 12)
        
        # Load image or create placeholder
        try:
            self.image = pygame.image.load(image_path)
            # Scale image to fit within the item, leaving space for label
            image_height = height - 20  # Leave 20px for label
            self.image = pygame.transform.scale(self.image, (width - 4, image_height - 4))
        except pygame.error:
            # Create placeholder
            image_height = height - 20
            self.image = pygame.Surface((width - 4, image_height - 4))
            self.image.fill(Colors.LIGHT_GRAY)
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.hover_color = Colors.LIGHT_BLUE
        self.text_color = Colors.BLACK
    
    def on_click(self):
        if self.callback:
            self.callback(self.label)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Background
        bg_color = self.hover_color if self.hover else self.bg_color
        pygame.draw.rect(surface, bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 1)
        
        # Image
        image_rect = self.image.get_rect()
        image_rect.centerx = self.rect.centerx
        image_rect.y = self.rect.y + 2
        surface.blit(self.image, image_rect)
        
        # Label
        text_surface = self.font.render(self.label, True, self.text_color)
        text_rect = text_surface.get_rect(center=(self.rect.centerx, self.rect.bottom - 10))
        surface.blit(text_surface, text_rect)

class ImageGallery(GUIComponent):
    """A collapsible gallery of image items"""
    
    def __init__(self, x: int, y: int, width: int, title: str, 
                 items: List[Tuple[str, str]], callback: Optional[Callable[[str], None]] = None):
        # Calculate height based on number of items
        self.title_height = 25
        self.item_width = 75
        self.item_height = 90
        self.items_per_row = max(1, (width - 10) // self.item_width)
        self.rows = (len(items) + self.items_per_row - 1) // self.items_per_row
        content_height = self.rows * self.item_height + 10
        
        super().__init__(x, y, width, self.title_height + content_height)
        
        self.title = title
        self.collapsed = False
        self.callback = callback
        self.font = pygame.font.Font(None, 14)
        self.image_items: List[ImageItem] = []
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.title_bg_color = Colors.LIGHT_GRAY
        self.title_text_color = Colors.BLACK
        
        # Create image items
        self._create_image_items(items)
        
        # Update rect for collapsed state
        self.expanded_height = self.rect.height
        self.collapsed_height = self.title_height
    
    def _create_image_items(self, items: List[Tuple[str, str]]):
        """Create image items from list of (image_path, label) tuples"""
        self.image_items.clear()
        
        for i, (image_path, label) in enumerate(items):
            row = i // self.items_per_row
            col = i % self.items_per_row
            
            item_x = self.rect.x + 5 + col * self.item_width
            item_y = self.rect.y + self.title_height + 5 + row * self.item_height
            
            item = ImageItem(item_x, item_y, self.item_width - 5, self.item_height - 5,
                           image_path, label, self.callback)
            self.image_items.append(item)
    
    def handle_event(self, event: pygame.event.Event) -> bool:
        if not self.visible or not self.enabled:
            return False
        
        # Handle title click for collapse/expand
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            title_rect = pygame.Rect(self.rect.x, self.rect.y, self.rect.width, self.title_height)
            if title_rect.collidepoint(event.pos):
                self.collapsed = not self.collapsed
                self.rect.height = self.collapsed_height if self.collapsed else self.expanded_height
                return True
        
        # Handle image item events if not collapsed
        if not self.collapsed:
            for item in self.image_items:
                if item.handle_event(event):
                    return True
        
        return False
    
    def update(self, dt: float):
        if not self.collapsed:
            for item in self.image_items:
                item.update(dt)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Draw background
        pygame.draw.rect(surface, self.bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 1)
        
        # Draw title bar
        title_rect = pygame.Rect(self.rect.x, self.rect.y, self.rect.width, self.title_height)
        pygame.draw.rect(surface, self.title_bg_color, title_rect)
        pygame.draw.rect(surface, self.border_color, title_rect, 1)
        
        # Draw title text with expand/collapse indicator
        indicator = "+" if self.collapsed else "-"
        title_text = f"{indicator} {self.title}"
        text_surface = self.font.render(title_text, True, self.title_text_color)
        text_rect = text_surface.get_rect(midleft=(title_rect.x + 5, title_rect.centery))
        surface.blit(text_surface, text_rect)
        
        # Draw items if not collapsed
        if not self.collapsed:
            # Create clipping rect for content area
            content_rect = pygame.Rect(self.rect.x + 1, self.rect.y + self.title_height + 1,
                                     self.rect.width - 2, self.rect.height - self.title_height - 2)
            clip_rect = surface.get_clip()
            surface.set_clip(content_rect)
            
            for item in self.image_items:
                item.draw(surface)
            
            surface.set_clip(clip_rect)

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
        
        # Galleries for each tab
        self.sound_galleries: List[ImageGallery] = []
        self.material_galleries: List[ImageGallery] = []
        
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
                                    self.rect.width - 10, "Voices", voices_items)
        self.sound_galleries.append(voices_gallery)
        
        # Material galleries  
        hvac_items = [
            ("assets/adult_male.png", "Item 1"),
            ("assets/adult_female.png", "Item 2"),
        ]
        
        electronics_items = [
            ("assets/adult_male.png", "Item 3"),
            ("assets/adult_female.png", "Item 4"),
        ]
        
        custom_items = [
            ("assets/adult_male.png", "Item 5"),
            ("assets/adult_female.png", "Item 6"),
        ]
        
        # Create galleries at initial positions (will be repositioned dynamically)
        hvac_gallery = ImageGallery(self.rect.x + 5, self.content_y + 5, 
                                  self.rect.width - 10, "HVAC", hvac_items)
        self.material_galleries.append(hvac_gallery)
        
        electronics_gallery = ImageGallery(self.rect.x + 5, self.content_y + 5, 
                                         self.rect.width - 10, "Electronics", electronics_items)
        self.material_galleries.append(electronics_gallery)
        
        custom_gallery = ImageGallery(self.rect.x + 5, self.content_y + 5, 
                                    self.rect.width - 10, "Custom", custom_items)
        self.material_galleries.append(custom_gallery)
        
        # Initial positioning
        self._reposition_galleries()
    
    def _reposition_galleries(self):
        """Reposition galleries to stack towards the top based on their collapsed state"""
        galleries = self.sound_galleries if self.active_tab == "SOUND" else self.material_galleries
        
        current_y = self.content_y + 5
        for gallery in galleries:
            old_y = gallery.rect.y
            gallery.rect.y = current_y
            
            # Update image item positions when gallery moves
            y_offset = current_y - old_y
            for item in gallery.image_items:
                item.rect.y += y_offset
            
            current_y += gallery.rect.height + 5
    
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

class RadioButton(GUIComponent):
    """A single radio button"""
    
    def __init__(self, x: int, y: int, width: int, height: int, text: str, 
                 group_id: str, selected: bool = False, callback: Optional[Callable[[str, bool], None]] = None):
        super().__init__(x, y, width, height)
        self.text = text
        self.group_id = group_id
        self.selected = selected
        self.callback = callback
        self.font = pygame.font.Font(None, 14)
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.selected_color = Colors.BLUE
        self.text_color = Colors.BLACK
        self.circle_size = 12
    
    def on_click(self):
        if not self.selected and self.callback:
            self.callback(self.group_id, True)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Draw radio circle
        circle_x = self.rect.x + 10
        circle_y = self.rect.centery
        
        # Outer circle
        pygame.draw.circle(surface, Colors.WHITE, (circle_x, circle_y), self.circle_size // 2)
        pygame.draw.circle(surface, self.border_color, (circle_x, circle_y), self.circle_size // 2, 2)
        
        # Inner circle if selected
        if self.selected:
            pygame.draw.circle(surface, self.selected_color, (circle_x, circle_y), self.circle_size // 2 - 4)
        
        # Text
        text_surface = self.font.render(self.text, True, self.text_color)
        text_rect = text_surface.get_rect(midleft=(circle_x + self.circle_size, circle_y))
        surface.blit(text_surface, text_rect)

class RadioButtonGroup(GUIComponent):
    """A group of mutually exclusive radio buttons"""
    
    def __init__(self, x: int, y: int, width: int, options: List[str], 
                 selected_index: int = 0, callback: Optional[Callable[[str], None]] = None):
        height = len(options) * 25
        super().__init__(x, y, width, height)
        
        self.options = options
        self.selected_index = selected_index
        self.callback = callback
        self.radio_buttons: List[RadioButton] = []
        
        # Create radio buttons
        for i, option in enumerate(options):
            button_y = y + i * 25
            radio = RadioButton(x, button_y, width, 25, option, "radio_group", 
                              i == selected_index, self._on_radio_click)
            self.radio_buttons.append(radio)
    
    def _on_radio_click(self, group_id: str, selected: bool):
        """Handle radio button click"""
        # Find which button was clicked and update selection
        clicked_radio = None
        for radio in self.radio_buttons:
            if radio.hover and not radio.selected:  # Only process if hovering and not already selected
                clicked_radio = radio
                break
        
        if clicked_radio:
            # Deselect all others
            for radio in self.radio_buttons:
                radio.selected = False
            # Select the clicked one
            clicked_radio.selected = True
            
            # Update selected index
            self.selected_index = self.radio_buttons.index(clicked_radio)
            
            if self.callback:
                self.callback(self.options[self.selected_index])
    
    def handle_event(self, event: pygame.event.Event) -> bool:
        if not self.visible or not self.enabled:
            return False
        
        # Handle mouse clicks on radio buttons
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            for i, radio in enumerate(self.radio_buttons):
                if radio.rect.collidepoint(event.pos) and not radio.selected:
                    # Deselect all others
                    for r in self.radio_buttons:
                        r.selected = False
                    # Select this one
                    radio.selected = True
                    self.selected_index = i
                    if self.callback:
                        self.callback(self.options[i])
                    return True
        
        # Handle hover states
        for radio in self.radio_buttons:
            radio.handle_event(event)
        
        return False
    
    def update(self, dt: float):
        for radio in self.radio_buttons:
            radio.update(dt)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        for radio in self.radio_buttons:
            radio.draw(surface)

class CheckBox(GUIComponent):
    """A checkbox toggle button"""
    
    def __init__(self, x: int, y: int, width: int, height: int, text: str, 
                 checked: bool = False, callback: Optional[Callable[[bool], None]] = None):
        super().__init__(x, y, width, height)
        self.text = text
        self.checked = checked
        self.callback = callback
        self.font = pygame.font.Font(None, 14)
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.checked_color = Colors.BLUE
        self.text_color = Colors.BLACK
        self.box_size = 12
    
    def on_click(self):
        self.checked = not self.checked
        if self.callback:
            self.callback(self.checked)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Draw checkbox square
        box_x = self.rect.x + 10
        box_y = self.rect.centery - self.box_size // 2
        box_rect = pygame.Rect(box_x, box_y, self.box_size, self.box_size)
        
        # Background
        pygame.draw.rect(surface, Colors.WHITE, box_rect)
        pygame.draw.rect(surface, self.border_color, box_rect, 2)
        
        # Checkmark if checked
        if self.checked:
            # Draw checkmark
            check_points = [
                (box_x + 3, box_y + 6),
                (box_x + 5, box_y + 8),
                (box_x + 9, box_y + 4)
            ]
            pygame.draw.lines(surface, self.checked_color, False, check_points, 2)
        
        # Text
        text_surface = self.font.render(self.text, True, self.text_color)
        text_rect = text_surface.get_rect(midleft=(box_x + self.box_size + 8, self.rect.centery))
        surface.blit(text_surface, text_rect)

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
        
        # Create GUI elements
        self._create_elements()
    
    def _create_elements(self):
        """Create all the property elements"""
        current_y = self.content_y + 10
        
        # Slider
        self.slider_label = TextButton(self.rect.x + 10, current_y, 60, 20, "Slider", 14)
        current_y += 25
        self.slider_value_label = TextButton(self.rect.x + self.rect.width - 30, current_y - 25, 20, 20, "1.2", 14)
        self.slider = Slider(self.rect.x + 10, current_y, self.rect.width - 60, 20, 0.0, 2.0, 1.2, self.on_slider_change)
        current_y += 35
        
        # Single options (radio buttons)
        self.single_options_label = TextButton(self.rect.x + 10, current_y, 100, 20, "Single options", 14)
        current_y += 25
        
        options = ["Option 1", "Option 2", "Option 3"]
        self.radio_group = RadioButtonGroup(self.rect.x + 10, current_y, self.rect.width - 20, 
                                          options, 0, self.on_radio_select)
        current_y += len(options) * 25 + 10
        
        # Dropdown
        self.dropdown_label = TextButton(self.rect.x + 10, current_y, 80, 20, "Drop-down", 14)
        current_y += 25
        self.dropdown = DropdownMenu(self.rect.x + 10, current_y, self.rect.width - 20, 25, 
                                   ["Text", "Option A", "Option B"], callback=self.on_dropdown_select)
        current_y += 35
        
        # Toggle (OFF)
        self.toggle_off_label = TextButton(self.rect.x + 10, current_y, 80, 20, "Toggle (OFF)", 14)
        current_y += 25
        self.toggle_off_checkbox = CheckBox(self.rect.x + 10, current_y, self.rect.width - 20, 25, 
                                          "Option 1", False, callback=self.on_toggle_off)
        current_y += 35
        
        # Toggle (ON)
        self.toggle_on_label = TextButton(self.rect.x + 10, current_y, 80, 20, "Toggle (ON)", 14)
        current_y += 25
        self.toggle_on_checkbox = CheckBox(self.rect.x + 10, current_y, self.rect.width - 20, 25, 
                                         "Option 1", True, callback=self.on_toggle_on)
        
        # Store all components for easy handling
        self.components = [
            self.slider_label, self.slider_value_label, self.slider,
            self.single_options_label, self.radio_group,
            self.dropdown_label, self.dropdown,
            self.toggle_off_label, self.toggle_off_checkbox,
            self.toggle_on_label, self.toggle_on_checkbox
        ]
    
    def on_slider_change(self, value):
        self.slider_value_label.text = f"{value:.1f}"
        print(f"Slider value: {value:.2f}")
    
    def on_radio_select(self, option):
        print(f"Radio selected: {option}")
    
    def on_dropdown_select(self, option):
        print(f"Dropdown selected: {option}")
    
    def on_toggle_off(self, state):
        print(f"Toggle OFF: {state}")
    
    def on_toggle_on(self, state):
        print(f"Toggle ON: {state}")
    
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
        """Create asset galleries"""
        # Sample room items
        room_items = [
            ("assets/adult_male.png", "Room 1"),
            ("assets/adult_female.png", "Room 2"),
        ]
        
        self.room_gallery = ImageGallery(self.rect.x + 5, self.content_y + 5, 
                                       self.rect.width - 10, "Room 1", room_items, self.on_asset_select)
        
        self.galleries = [self.room_gallery]
    
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
            
            # Update image item positions
            y_offset = current_y - old_y
            for item in gallery.image_items:
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

class MainApplication:
    """Main application class that demonstrates all GUI components"""
    
    def __init__(self, width: int = 1200, height: int = 800):
        self.width = width
        self.height = height
        
        # Set up OpenGL context
        pygame.display.gl_set_attribute(pygame.GL_DOUBLEBUFFER, 1)
        pygame.display.gl_set_attribute(pygame.GL_DEPTH_SIZE, 24)
        
        self.screen = pygame.display.set_mode((width, height), pygame.OPENGL | pygame.DOUBLEBUF)
        pygame.display.set_caption("3D Architecture GUI")
        
        # Basic OpenGL setup - will be configured properly by Render3 class
        # Don't set up OpenGL state here to avoid conflicts
        
        self.clock = pygame.time.Clock()
        self.running = True
        
        # Initialize 3D renderer
        self.renderer = None
        self.viewport_rect = None
        self.init_3d_renderer()
        
        # Initialize GUI components
        self.init_gui()
    
    def init_3d_renderer(self):
        """Initialize the 3D renderer"""
        # Define viewport area (where 3D content will be rendered)
        self.viewport_rect = pygame.Rect(200, 90, self.width - 400, self.height - 220)
        
        # Load a single specific STL file
        stl_file = "resources/cube_hollow.stl"
        
        if os.path.exists(stl_file):
            try:
                # Import Render3 class
                from render3 import Render3
                self.renderer = Render3(stl_file, self.viewport_rect, self.height)
                print(f"Successfully loaded 3D model: {stl_file}")
            except Exception as e:
                print(f"Error loading 3D renderer: {e}")
                self.renderer = None
        else:
            print(f"STL file not found: {stl_file}")
            self.renderer = None
    
    def init_gui(self):
        """Initialize all GUI components"""
        self.components: List[GUIComponent] = []
        
        # Menu bar at the very top
        self.menu_bar = MenuBar(0, 0, self.width, 25)
        
        # Add Settings menu
        settings_menu = [
            ("Preferences", self.on_preferences),
            ("Display Settings", self.on_display_settings),
            ("Audio Settings", self.on_audio_settings),
            ("Keyboard Shortcuts", self.on_keyboard_shortcuts),
        ]
        self.menu_bar.add_menu("Settings", settings_menu)
        
        # Add File menu
        file_menu = [
            ("New Project", self.on_new_project),
            ("Open Project", self.on_open_project),
            ("Save Project", self.on_save_project),
            ("Save As...", self.on_save_as),
            ("Import...", self.on_import),
            ("Export...", self.on_export),
            ("Recent Projects", self.on_recent_projects),
            ("Exit", self.on_exit),
        ]
        self.menu_bar.add_menu("File", file_menu)
        
        # Add Edit menu
        edit_menu = [
            ("Undo", self.on_undo),
            ("Redo", self.on_redo),
            ("Cut", self.on_cut),
            ("Copy", self.on_copy),
            ("Paste", self.on_paste),
            ("Delete", self.on_delete),
            ("Select All", self.on_select_all),
            ("Find", self.on_find),
        ]
        self.menu_bar.add_menu("Edit", edit_menu)
        
        self.components.append(self.menu_bar)
        
        # Top toolbar buttons (moved down to accommodate menu bar)
        toolbar_y = 30  # Moved down from 10
        button_width = 60
        button_height = 50
        button_spacing = 5
        start_x = 50
        
        # Create toolbar buttons using the asset images
        toolbar_buttons = [
            ("Move", "assets/button_move.png", self.on_move_click),
            ("Copy", "assets/button_copy.png", self.on_copy_click),
            ("Cut", "assets/button_cut.png", self.on_cut_click),
            ("Paste", "assets/button_paste.png", self.on_paste_click),
            ("Delete", "assets/button_delete.png", self.on_delete_click),
            ("Measure", "assets/button_measure.png", self.on_measure_click),
        ]
        
        for i, (name, image_path, callback) in enumerate(toolbar_buttons):
            x = start_x + i * (button_width + button_spacing)
            button = ImageButton(x, toolbar_y, button_width, button_height, image_path, callback)
            self.components.append(button)
        
        # Left panel - Library (using new LibraryPanel)
        panel_y = 90  # Moved down from 80
        library_panel = LibraryPanel(10, panel_y, 180, 500)
        self.components.append(library_panel)
        
        # Right panels - Property and Assets (split into two)
        right_panel_x = self.width - 200
        
        # Property panel (top half) - needs more height for all elements
        property_height = 320  # Increased to fit all elements including toggles
        property_panel = PropertyPanel(right_panel_x, panel_y, 190, property_height)
        self.components.append(property_panel)
        
        # Assets panel (bottom half) - positioned much lower
        assets_y = panel_y + property_height + 20  # More spacing between panels
        assets_height = 180  # Remaining space (reduced to fit properly)
        assets_panel = AssetsPanel(right_panel_x, assets_y, 190, assets_height)
        self.components.append(assets_panel)
        
        # Bottom toolbar
        bottom_buttons = [
            ("Import Room", self.on_import_room),
            ("Place Sound", self.on_place_sound),
            ("Place Listener", self.on_place_listener),
            ("Render", self.on_render)
        ]
        
        button_width = 100
        total_width = len(bottom_buttons) * button_width + (len(bottom_buttons) - 1) * 10
        start_x = (self.width - total_width) // 2
        
        for i, (text, callback) in enumerate(bottom_buttons):
            x = start_x + i * (button_width + 10)
            button = TextButton(x, self.height - 40, button_width, 30, text, callback=callback)
            self.components.append(button)
    
    # Menu callback methods
    def on_preferences(self): print("Preferences")
    def on_display_settings(self): print("Display Settings")
    def on_audio_settings(self): print("Audio Settings")
    def on_keyboard_shortcuts(self): print("Keyboard Shortcuts")
    
    def on_new_project(self): print("New Project")
    def on_open_project(self): print("Open Project")
    def on_save_project(self): print("Save Project")
    def on_save_as(self): print("Save As...")
    def on_import(self): print("Import...")
    def on_export(self): print("Export...")
    def on_recent_projects(self): print("Recent Projects")
    def on_exit(self): 
        print("Exit")
        self.running = False
    
    def on_undo(self): print("Undo")
    def on_redo(self): print("Redo")
    def on_cut(self): print("Cut")
    def on_copy(self): print("Copy")
    def on_paste(self): print("Paste")
    def on_delete(self): print("Delete")
    def on_select_all(self): print("Select All")
    def on_find(self): print("Find")
    
    # Toolbar callback methods
    def on_move_click(self): print("Move tool selected")
    def on_copy_click(self): print("Copy tool selected")
    def on_cut_click(self): print("Cut tool selected") 
    def on_paste_click(self): print("Paste tool selected")
    def on_delete_click(self): print("Delete tool selected")
    def on_measure_click(self): print("Measure tool selected")
    
    # Panel callback methods
    def on_library_item_select(self, item): print(f"Library item selected: {item}")
    def on_import_room(self): print("Import room")
    def on_place_sound(self): print("Place sound")
    def on_place_listener(self): print("Place listener")
    def on_render(self): print("Render")
    
    def handle_events(self):
        """Handle all pygame events"""
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            
            # Check if event is in 3D viewport area
            viewport_event = False
            if hasattr(event, 'pos') and self.viewport_rect and self.renderer:
                if self.viewport_rect.collidepoint(event.pos):
                    viewport_event = True
                    # Route event to 3D renderer
                    self.renderer.check_keybinds(event)
            
            # Let GUI components handle events (if not consumed by 3D viewport)
            if not viewport_event:
                for component in self.components:
                    if component.handle_event(event):
                        break  # Stop processing if event was consumed
            
            # Handle keyboard events for 3D renderer (not position-dependent)
            if event.type == pygame.KEYDOWN and self.renderer:
                self.renderer.check_keybinds(event)
    
    def update(self, dt: float):
        """Update all components"""
        for component in self.components:
            component.update(dt)
    
    def draw(self):
        """Draw everything with mixed OpenGL 3D and 2D GUI"""
        # Clear the screen
        glClearColor(1.0, 1.0, 1.0, 1.0)  # White background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        
        # Render 3D scene first (if renderer is available)
        if self.renderer:
            self.renderer.draw_scene()
        else:
            # Draw placeholder viewport area
            self.draw_placeholder_viewport()
        
        # Switch to 2D rendering for GUI
        self.setup_2d_rendering()
        
        # Create a pygame surface for 2D GUI rendering
        gui_surface = pygame.Surface((self.width, self.height), pygame.SRCALPHA)
        gui_surface.fill((0, 0, 0, 0))  # Transparent background
        
        # Draw all GUI components on the surface (without any dropdowns)
        for component in self.components:
            if isinstance(component, MenuBar):
                component.draw_base(gui_surface)
            elif isinstance(component, PropertyPanel):
                component.draw_base(gui_surface)
            else:
                component.draw(gui_surface)
        
        # Draw all dropdowns last (on top of everything)
        for component in self.components:
            if isinstance(component, MenuBar):
                component.draw_dropdowns(gui_surface)
            elif isinstance(component, PropertyPanel):
                component.draw_dropdowns(gui_surface)
        
        # Blit the GUI surface to OpenGL
        self.blit_surface_to_opengl(gui_surface)
        
        pygame.display.flip()
    
    def draw_placeholder_viewport(self):
        """Draw a placeholder when 3D renderer is not available"""
        # Set up 2D rendering temporarily
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glLoadIdentity()
        glOrtho(0, self.width, self.height, 0, -1, 1)
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glLoadIdentity()
        
        # Draw viewport background
        glColor3f(0.8, 0.8, 0.8)  # Light gray
        glBegin(GL_QUADS)
        glVertex2f(self.viewport_rect.x, self.viewport_rect.y)
        glVertex2f(self.viewport_rect.right, self.viewport_rect.y)
        glVertex2f(self.viewport_rect.right, self.viewport_rect.bottom)
        glVertex2f(self.viewport_rect.x, self.viewport_rect.bottom)
        glEnd()
        
        # Draw border
        glColor3f(0.3, 0.3, 0.3)  # Dark gray
        glLineWidth(2)
        glBegin(GL_LINE_LOOP)
        glVertex2f(self.viewport_rect.x, self.viewport_rect.y)
        glVertex2f(self.viewport_rect.right, self.viewport_rect.y)
        glVertex2f(self.viewport_rect.right, self.viewport_rect.bottom)
        glVertex2f(self.viewport_rect.x, self.viewport_rect.bottom)
        glEnd()
        
        # Restore matrices
        glPopMatrix()
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()
        glMatrixMode(GL_MODELVIEW)
    
    def setup_2d_rendering(self):
        """Set up OpenGL for 2D GUI rendering"""
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glLoadIdentity()
        glOrtho(0, self.width, self.height, 0, -1, 1)
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glLoadIdentity()
        glDisable(GL_DEPTH_TEST)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    
    def blit_surface_to_opengl(self, surface):
        """Convert pygame surface to OpenGL texture and render it"""
        # Convert surface to string data
        w, h = surface.get_size()
        raw = pygame.image.tostring(surface, 'RGBA')
        
        # Create and bind texture
        texture_id = glGenTextures(1)
        glBindTexture(GL_TEXTURE_2D, texture_id)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        
        # Enable texturing and render
        glEnable(GL_TEXTURE_2D)
        glColor4f(1, 1, 1, 1)
        glBegin(GL_QUADS)
        glTexCoord2f(0, 0); glVertex2f(0, 0)
        glTexCoord2f(1, 0); glVertex2f(w, 0)
        glTexCoord2f(1, 1); glVertex2f(w, h)
        glTexCoord2f(0, 1); glVertex2f(0, h)
        glEnd()
        glDisable(GL_TEXTURE_2D)
        
        # Clean up
        glDeleteTextures([texture_id])
        
        # Restore matrices
        glPopMatrix()
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()
        glMatrixMode(GL_MODELVIEW)
        glEnable(GL_DEPTH_TEST)
    
    def run(self):
        """Main game loop"""
        while self.running:
            dt = self.clock.tick(60) / 1000.0  # Delta time in seconds
            
            self.handle_events()
            self.update(dt)
            self.draw()
        
        pygame.quit()

if __name__ == "__main__":
    app = MainApplication()
    app.run()
