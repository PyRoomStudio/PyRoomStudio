"""
PyRoomStudio - 3D Acoustic Simulation GUI
Main entry point for the application.
"""
import pygame  # pyright: ignore[reportMissingImports]
from utils import resource_path

# Initialize pygame
pygame.init()

# Set window icon
logo_image = pygame.image.load(resource_path("logo.ico"))
pygame.display.set_icon(logo_image)

# Import and run the application
from gui import MainApplication

if __name__ == "__main__":
    app = MainApplication()
    app.run()
