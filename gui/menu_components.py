"""
Menu GUI components: MenuItem, MenuButton, MenuBar
"""
import pygame
from typing import List, Tuple, Callable, Optional, Union
from .constants import Colors
from .base_components import GUIComponent


class MenuItem(GUIComponent):
    """A single item in a dropdown menu"""
    
    def __init__(self, x: int, y: int, width: int, height: int, text: str,
                 callback: Optional[Callable] = None, font_size: int = 14,
                 tooltip: Optional[str] = None):
        super().__init__(x, y, width, height, tooltip)
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
        if self.enabled:
            bg_color = self.hover_color if self.hover else self.bg_color
            text_color = self.text_color
        else:
            # Grayed out appearance when disabled
            bg_color = self.bg_color
            text_color = Colors.GRAY
        
        pygame.draw.rect(surface, bg_color, self.rect)
        pygame.draw.rect(surface, self.border_color, self.rect, 1)
        
        # Text
        text_surface = self.font.render(self.text, True, text_color)
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
        self.menus: List[Tuple[str, List[Union[Tuple[str, Callable], Tuple[str, Callable, Optional[str], bool]]]]] = []
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
    
    def add_menu(self, title: str, items: List[Union[Tuple[str, Callable], Tuple[str, Callable, Optional[str], bool]]]):
        """Add a menu with title and list of (item_name, callback) or (item_name, callback, tooltip, enabled) tuples"""
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
            
            for i, item_data in enumerate(items):
                # Support both old format (2 values) and new format (4 values)
                if len(item_data) == 2:
                    item_text, callback = item_data
                    tooltip = None
                    enabled = True
                else:
                    item_text, callback, tooltip, enabled = item_data
                
                item_y = start_y + i * self.menu_item_height
                
                def item_callback(cb=callback):
                    cb()
                    self.active_menu_index = -1  # Close menu after selection
                    self.dropdown_items.clear()
                
                menu_item = MenuItem(button.rect.x, item_y, self.dropdown_width, 
                                   self.menu_item_height, item_text, item_callback, tooltip=tooltip)
                menu_item.enabled = enabled
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
