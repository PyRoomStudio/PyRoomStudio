"""
Gallery GUI components: ImageItem, SurfaceItem, ImageGallery, SurfaceGallery
"""
import pygame
from typing import List, Tuple, Optional, Callable
from .constants import Colors
from .base_components import GUIComponent


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


class SurfaceItem(GUIComponent):
    """A surface item showing the color/texture of a 3D model surface"""
    
    def __init__(self, x: int, y: int, width: int, height: int, 
                 surface_name: str, surface_color: Tuple[int, int, int], 
                 surface_index: int, callback: Optional[Callable] = None):
        super().__init__(x, y, width, height)
        self.surface_name = surface_name
        self.surface_color = surface_color
        self.surface_index = surface_index
        self.callback = callback
        self.font = pygame.font.Font(None, 12)
        
        # Colors
        self.border_color = Colors.DARK_GRAY
        self.hover_color = Colors.LIGHT_BLUE
        self.text_color = Colors.BLACK
    
    def update_color(self, new_color: Tuple[int, int, int]):
        """Update the surface color"""
        self.surface_color = new_color
    
    def on_click(self):
        if self.callback:
            self.callback(self.surface_index, self.surface_name)
    
    def draw(self, surface: pygame.Surface):
        if not self.visible:
            return
        
        # Background with hover effect
        if self.hover:
            pygame.draw.rect(surface, self.hover_color, self.rect)
        
        # Draw border
        pygame.draw.rect(surface, self.border_color, self.rect, 1)
        
        # Draw color square (most of the item)
        color_rect = pygame.Rect(self.rect.x + 2, self.rect.y + 2, 
                               self.rect.width - 4, self.rect.height - 22)
        
        # Convert float color (0-1) to int color (0-255) if needed
        if all(isinstance(c, float) for c in self.surface_color):
            display_color = tuple(int(c * 255) for c in self.surface_color)
        else:
            display_color = self.surface_color
        
        pygame.draw.rect(surface, display_color, color_rect)
        pygame.draw.rect(surface, self.border_color, color_rect, 1)
        
        # Draw surface name
        text_surface = self.font.render(self.surface_name, True, self.text_color)
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


class SurfaceGallery(GUIComponent):
    """A collapsible gallery of surface items with scrolling support"""
    
    def __init__(self, x: int, y: int, width: int, title: str, 
                 surfaces: List[Tuple[str, Tuple[int, int, int], int]], 
                 max_height: int = 300,
                 callback: Optional[Callable[[int, str], None]] = None):
        self.title_height = 25
        self.item_width = 75
        self.item_height = 90
        self.items_per_row = max(1, (width - 20) // self.item_width)  # Account for scrollbar
        self.rows = (len(surfaces) + self.items_per_row - 1) // self.items_per_row
        
        # Calculate content dimensions
        full_content_height = self.rows * self.item_height + 10
        self.max_content_height = max_height - self.title_height
        self.needs_scrolling = full_content_height > self.max_content_height
        
        # Set actual height
        if self.needs_scrolling:
            actual_height = self.title_height + self.max_content_height
        else:
            actual_height = self.title_height + full_content_height
            
        super().__init__(x, y, width, actual_height)
        
        self.title = title
        self.collapsed = False
        self.callback = callback
        self.font = pygame.font.Font(None, 14)
        self.surface_items: List[SurfaceItem] = []
        
        # Scrolling properties
        self.scroll_y = 0
        self.max_scroll = max(0, full_content_height - self.max_content_height)
        self.scrollbar_width = 15
        self.scrollbar_dragging = False
        self.scrollbar_drag_offset = 0
        
        # Colors
        self.bg_color = Colors.WHITE
        self.border_color = Colors.DARK_GRAY
        self.title_bg_color = Colors.LIGHT_GRAY
        self.title_text_color = Colors.BLACK
        self.scrollbar_color = Colors.LIGHT_GRAY
        self.scrollbar_handle_color = Colors.DARK_GRAY
        
        # Create surface items
        self._create_surface_items(surfaces)
        
        # Update rect for collapsed state
        self.expanded_height = self.rect.height
        self.collapsed_height = self.title_height
    
    def _create_surface_items(self, surfaces: List[Tuple[str, Tuple[int, int, int], int]]):
        """Create surface items from list of (name, color, index) tuples"""
        self.surface_items.clear()
        
        content_width = self.rect.width - (self.scrollbar_width if self.needs_scrolling else 0) - 10
        items_per_row = max(1, content_width // self.item_width)
        
        for i, (surface_name, surface_color, surface_index) in enumerate(surfaces):
            row = i // items_per_row
            col = i % items_per_row
            
            item_x = self.rect.x + 5 + col * self.item_width
            item_y = self.rect.y + self.title_height + 5 + row * self.item_height
            
            item = SurfaceItem(item_x, item_y, self.item_width - 5, self.item_height - 5,
                             surface_name, surface_color, surface_index, self.callback)
            self.surface_items.append(item)
    
    def update_surface_color(self, surface_index: int, new_color: Tuple[int, int, int]):
        """Update the color of a specific surface"""
        for item in self.surface_items:
            if item.surface_index == surface_index:
                item.update_color(new_color)
                break
    
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
            
            # Handle scrollbar dragging
            if not self.collapsed and self.needs_scrolling:
                scrollbar_rect = self._get_scrollbar_rect()
                if scrollbar_rect.collidepoint(event.pos):
                    self.scrollbar_dragging = True
                    self.scrollbar_drag_offset = event.pos[1] - self._get_scrollbar_handle_rect().y
                    return True
        
        elif event.type == pygame.MOUSEBUTTONUP and event.button == 1:
            self.scrollbar_dragging = False
        
        elif event.type == pygame.MOUSEMOTION and self.scrollbar_dragging:
            # Update scroll position based on mouse drag
            scrollbar_rect = self._get_scrollbar_rect()
            handle_height = self._get_scrollbar_handle_height()
            
            new_handle_y = event.pos[1] - self.scrollbar_drag_offset
            min_y = scrollbar_rect.y
            max_y = scrollbar_rect.bottom - handle_height
            
            handle_y = max(min_y, min(max_y, new_handle_y))
            scroll_ratio = (handle_y - min_y) / (max_y - min_y) if max_y > min_y else 0
            self.scroll_y = int(scroll_ratio * self.max_scroll)
            return True
        
        elif event.type == pygame.MOUSEWHEEL and not self.collapsed:
            # Handle mouse wheel scrolling
            content_rect = pygame.Rect(self.rect.x, self.rect.y + self.title_height,
                                     self.rect.width, self.rect.height - self.title_height)
            mouse_pos = pygame.mouse.get_pos()
            if content_rect.collidepoint(mouse_pos):
                scroll_amount = -event.y * 30  # Scroll speed
                self.scroll_y = max(0, min(self.max_scroll, self.scroll_y + scroll_amount))
                return True
        
        # Handle surface item events if not collapsed
        if not self.collapsed:
            # Adjust item positions for scrolling
            for item in self.surface_items:
                # Temporarily adjust item position for scrolling
                original_y = item.rect.y
                item.rect.y = original_y - self.scroll_y
                
                if item.handle_event(event):
                    item.rect.y = original_y  # Restore position
                    return True
                
                item.rect.y = original_y  # Restore position
        
        return False
    
    def _get_scrollbar_rect(self):
        """Get the scrollbar track rectangle"""
        return pygame.Rect(
            self.rect.right - self.scrollbar_width,
            self.rect.y + self.title_height,
            self.scrollbar_width,
            self.rect.height - self.title_height
        )
    
    def _get_scrollbar_handle_height(self):
        """Calculate scrollbar handle height based on content ratio"""
        if self.max_scroll == 0:
            return self.rect.height - self.title_height
        
        content_ratio = self.max_content_height / (self.max_content_height + self.max_scroll)
        return max(20, int((self.rect.height - self.title_height) * content_ratio))
    
    def _get_scrollbar_handle_rect(self):
        """Get the scrollbar handle rectangle"""
        scrollbar_rect = self._get_scrollbar_rect()
        handle_height = self._get_scrollbar_handle_height()
        
        if self.max_scroll == 0:
            handle_y = scrollbar_rect.y
        else:
            scroll_ratio = self.scroll_y / self.max_scroll
            handle_y = scrollbar_rect.y + int(scroll_ratio * (scrollbar_rect.height - handle_height))
        
        return pygame.Rect(
            scrollbar_rect.x,
            handle_y,
            self.scrollbar_width,
            handle_height
        )
    
    def update(self, dt: float):
        if not self.collapsed:
            for item in self.surface_items:
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
            # Create clipping rect for content area (excluding scrollbar)
            content_width = self.rect.width - (self.scrollbar_width if self.needs_scrolling else 0) - 2
            content_rect = pygame.Rect(self.rect.x + 1, self.rect.y + self.title_height + 1,
                                     content_width, self.rect.height - self.title_height - 2)
            clip_rect = surface.get_clip()
            surface.set_clip(content_rect)
            
            # Draw items with scroll offset
            for item in self.surface_items:
                # Temporarily adjust item position for scrolling
                original_y = item.rect.y
                item.rect.y = original_y - self.scroll_y
                
                # Only draw if item is visible in the clipped area
                if (item.rect.bottom > content_rect.y and 
                    item.rect.y < content_rect.bottom):
                    item.draw(surface)
                
                # Restore original position
                item.rect.y = original_y
            
            surface.set_clip(clip_rect)
            
            # Draw scrollbar if needed
            if self.needs_scrolling:
                scrollbar_rect = self._get_scrollbar_rect()
                handle_rect = self._get_scrollbar_handle_rect()
                
                # Draw scrollbar track
                pygame.draw.rect(surface, self.scrollbar_color, scrollbar_rect)
                pygame.draw.rect(surface, self.border_color, scrollbar_rect, 1)
                
                # Draw scrollbar handle
                pygame.draw.rect(surface, self.scrollbar_handle_color, handle_rect)
                pygame.draw.rect(surface, self.border_color, handle_rect, 1)
