"""
Base GUI components: GUIComponent, TextButton, ImageButton, ToggleButton
"""
import pygame
from typing import Optional, Callable
from .constants import Colors


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
