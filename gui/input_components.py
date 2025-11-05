"""
Input GUI components: DropdownMenu, Slider, RadioButton, CheckBox
"""
import pygame
from typing import List, Optional, Callable
from .constants import Colors
from .base_components import GUIComponent


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
