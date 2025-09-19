import pygame
import pygame_gui


class MainApp:
    def __init__(self):
        pygame.init()
        self.window_size = (1440, 810)
        self.screen = pygame.display.set_mode(self.window_size)
        pygame.display.set_caption("3D Acoustic Room Modeler")

        self.ui_manager = pygame_gui.UIManager(self.window_size, 'theme.json')
        self.background = pygame.Surface(self.window_size)
        self.background.fill(self.ui_manager.get_theme().get_colour('dark_bg'))

        self.setup_layout()

        self.is_running = True
        self.clock = pygame.time.Clock()

    def setup_layout(self):
        # Top toolbar
        self.top_bar = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((0, 0), (self.window_size[0], 80)),
            manager=self.ui_manager)
        
        # Menu bar
        self.menu_bar = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((0, 0), (self.window_size[0], 30)),
            text="Settings   File   Edit",
            manager=self.ui_manager,
            container=self.top_bar)
        
        # Toolbar buttons
        toolbar_buttons = [
            ("Move", "move_button"),
            ("Copy", "copy_button"),
            ("Cut", "cut_button"),
            ("Paste", "paste_button"),
            ("Delete", "delete_button"),
            ("Measure", "measure_button")
        ]
        
        button_width = 60
        button_height = 40
        x_start = 10
        y_pos = 35
        
        for i, (text, object_id) in enumerate(toolbar_buttons):
            pygame_gui.elements.UIButton(
                relative_rect=pygame.Rect((x_start + i * (button_width + 5), y_pos), (button_width, button_height)),
                text=text,
                manager=self.ui_manager,
                container=self.top_bar,
                object_id=f"#{object_id}")
        
        # Left Panel
        self.left_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((0, 80), (250, self.window_size[1] - 140)),
            manager=self.ui_manager,
            object_id="#left_panel")
            
        # Library Label
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((10, 5), (100, 30)),
            text="LIBRARY",
            manager=self.ui_manager,
            container=self.left_panel)
            
        # Sound/Material Tabs
        self.sound_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((10, 40), (70, 30)),
            text="SOUND",
            manager=self.ui_manager,
            container=self.left_panel)
        self.material_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((85, 40), (85, 30)),
            text="MATERIAL",
            manager=self.ui_manager,
            container=self.left_panel)
            
        # Voices section
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((10, 80), (100, 30)),
            text="+ Voices",
            manager=self.ui_manager,
            container=self.left_panel)
            
        voices = [
            ("adult_male", "resources/icons/adult_male.png"),
            ("adult_female", "resources/icons/adult_female.png")
        ]
        
        icon_size = (64, 64)
        x_padding = 15
        y_padding = 115
        
        for i, (name, path) in enumerate(voices):
            row = i // 2
            col = i % 2
            
            x = x_padding + col * (icon_size[0] + 10)
            y = y_padding + row * (icon_size[1] + 20)
            
            try:
                image_surface = pygame.image.load(path).convert_alpha()
                image_surface = pygame.transform.scale(image_surface, icon_size)
                
                button_rect = pygame.Rect((x, y), icon_size)
                
                pygame_gui.elements.UIButton(
                    relative_rect=button_rect,
                    text="",
                    manager=self.ui_manager,
                    container=self.left_panel,
                    object_id=f"#{name}_button"
                )
                
                pygame_gui.elements.UIImage(
                    relative_rect=button_rect,
                    image_surface=image_surface,
                    manager=self.ui_manager,
                    container=self.left_panel
                )
            except pygame.error:
                print(f"Warning: Could not load image at {path}")
                # Optional: create a placeholder button if image fails to load
                button_rect = pygame.Rect((x, y), icon_size)
                pygame_gui.elements.UIButton(
                    relative_rect=button_rect,
                    text=name, # show name if image is missing
                    manager=self.ui_manager,
                    container=self.left_panel,
                    object_id=f"#{name}_button"
                )


        # Right Panel
        self.right_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((self.window_size[0] - 300, 80), (300, self.window_size[1] - 140)),
            manager=self.ui_manager)
            
        # Property and Assets labels
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((10, 5), (100, 30)),
            text="PROPERTY",
            manager=self.ui_manager,
            container=self.right_panel)
            
        # Placeholder for property elements
        # pygame_gui.elements.UILabel(
        #     relative_rect=pygame.Rect((10, 40), (280, 200)),
        #     text="Slider, options, etc. here",
        #     manager=self.ui_manager,
        #     container=self.right_panel)

        # Slider
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect(10, 40, 50, 30), text="Slider",
            manager=self.ui_manager, container=self.right_panel)
        self.slider = pygame_gui.elements.UIHorizontalSlider(
            relative_rect=pygame.Rect(70, 40, 150, 30),
            start_value=1.2, value_range=(0.0, 10.0),
            manager=self.ui_manager, container=self.right_panel)
        self.slider_value_label = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect(230, 40, 50, 30), text="1.2",
            manager=self.ui_manager, container=self.right_panel)
            
        # Single options (Radio Buttons)
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect(10, 80, 150, 30), text="Single options",
            manager=self.ui_manager, container=self.right_panel)
        self.option1_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect(20, 110, 100, 30), text="Option 1",
            manager=self.ui_manager, container=self.right_panel)
        self.option2_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect(20, 140, 100, 30), text="Option 2",
            manager=self.ui_manager, container=self.right_panel)
        self.option3_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect(20, 170, 100, 30), text="Option 3",
            manager=self.ui_manager, container=self.right_panel)
            
        # Dropdown
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect(10, 210, 100, 30), text="Drop-down",
            manager=self.ui_manager, container=self.right_panel)
        self.dropdown = pygame_gui.elements.UIDropDownMenu(
            options_list=["Text", "Option A", "Option B"],
            starting_option="Text",
            relative_rect=pygame.Rect(120, 210, 150, 30),
            manager=self.ui_manager, container=self.right_panel)
            
        # Toggles
        self.toggle_off_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect(20, 250, 150, 30), text="Toggle (OFF)",
            manager=self.ui_manager, container=self.right_panel)
        self.toggle_on_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect(20, 290, 150, 30), text="Toggle (ON)",
            manager=self.ui_manager, container=self.right_panel)
            
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((10, 330), (100, 30)),
            text="ASSETS",
            manager=self.ui_manager,
            container=self.right_panel)

        # Placeholder for asset elements
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((10, 360), (280, 100)),
            text="+ Room 1",
            manager=self.ui_manager,
            container=self.right_panel)
            
        # Bottom Panel
        self.bottom_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((0, self.window_size[1] - 60), (self.window_size[0], 60)),
            manager=self.ui_manager)

        # Bottom toolbar buttons
        bottom_buttons = [
            ("Import Room", "import_room"),
            ("Place Sound", "place_sound"),
            ("Place Listener", "place_listener"),
            ("Render", "render")
        ]
        
        b_button_width = 150
        b_button_height = 40
        b_x_start = (self.window_size[0] - (len(bottom_buttons) * (b_button_width + 10) - 10)) // 2
        b_y_pos = 10
        
        for i, (text, object_id) in enumerate(bottom_buttons):
            pygame_gui.elements.UIButton(
                relative_rect=pygame.Rect((b_x_start + i * (b_button_width + 10), b_y_pos), (b_button_width, b_button_height)),
                text=text,
                manager=self.ui_manager,
                container=self.bottom_panel,
                object_id=f"#{object_id}")

        # Center panel (for 3D view)
        self.center_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((250, 80), (self.window_size[0] - 550, self.window_size[1] - 140)),
            manager=self.ui_manager)

    def process_events(self, event):
        self.ui_manager.process_events(event)
        
        if event.type == pygame_gui.UI_HORIZONTAL_SLIDER_MOVED:
            if event.ui_element == self.slider:
                self.slider_value_label.set_text(f"{self.slider.get_current_value():.1f}")


    def run(self):
        while self.is_running:
            time_delta = self.clock.tick(60) / 1000.0

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.is_running = False

                self.process_events(event)

            self.ui_manager.update(time_delta)

            self.screen.blit(self.background, (0, 0))
            self.ui_manager.draw_ui(self.screen)

            pygame.display.update()

        pygame.quit()


if __name__ == "__main__":
    app = MainApp()
    app.run()
