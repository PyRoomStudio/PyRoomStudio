"""
PyRoomStudio - 3D Acoustic Simulation GUI
Main entry point for the application.
"""
import pygame
import os

# Initialize pygame
pygame.init()

# Set window icon
logo_image = pygame.image.load("assets/logo_v3.svg")
pygame.display.set_icon(logo_image)

# Import and run the application
from gui import MainApplication

if __name__ == "__main__":
    app = MainApplication()
    app.run()
