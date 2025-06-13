from imgui_bundle import imgui, immapp
from imgui_bundle import ImVec2, ImVec4
from imgui_bundle import hello_imgui, imgui_color_text_edit as ed

import pandas as pd

from typing import List

# Struct that holds the application's state
class AppState:
    counter: int
    current_ingredients: str
    current_directions: str
    #title_font: imgui.ImFont
    #color_font: imgui.ImFont
    #emoji_font: imgui.ImFont
    #large_icon_font: imgui.ImFont

    def __init__(self):
        self.counter = 0
        self.current_ingredients = ""
        self.current_directions = ""

df = pd.read_csv("recipes.csv")

# Drop all but one instance of each duplicate
df.drop_duplicates(subset=['recipe_name'], keep='first', inplace=True)

items = df['recipe_name']
item_ingredients = df['ingredients']
item_directions = df['directions']

current_ingredients = "Test1"
current_directions = "Test2"

def show_search_window(app_state: AppState):
    static = show_search_window
    if imgui.tree_node("Recipes"):
        # Index of the currently selected item
        if not hasattr(static, 'item_current_idx'):
            static.item_current_idx = 0

        # Custom size list box: full width and 5 items tall
        full_width = -imgui.FLT_MIN
        item_height = imgui.get_text_line_height_with_spacing()
        if imgui.begin_list_box("##listbox 2", ImVec2(full_width, 15 * item_height)):
            for n, item in enumerate(items):
                # Determine if the current item is selected
                is_selected = (static.item_current_idx == n)
                if imgui.selectable(item, is_selected):
                    static.item_current_idx = n
                # Set the default focus to the selected item
                if is_selected:
                    imgui.set_item_default_focus()
                    app_state.current_ingredients = item_ingredients[n]
                    app_state.current_directions = item_directions[n]
                    
            imgui.end_list_box()

            imgui.tree_pop()


'''
def show_search_window(app_state: AppState):
    static = show_search_window
    if imgui.tree_node("Recipes"):
        
        for n, item in enumerate(items):
            if imgui.collapsing_header(item, imgui.TreeNodeFlags_.none.value):
                app_state.current_ingredients = item_ingredients[n]
                app_state.current_directions = item_directions[n]
        imgui.tree_pop()
'''

def show_display_window(app_state: AppState):
    static = show_display_window
    imgui.text(app_state.current_ingredients)
    imgui.text(app_state.current_directions)

# Helper method to create dockspace split
def create_docking_split() -> hello_imgui.DockingSplit:
    split = hello_imgui.DockingSplit(
        initial_dock_="MainDockSpace",
        direction_=imgui.Dir.left,
        ratio_=0.5,
        new_dock_="Recipe Database"
    )
    
    return split

# Helper method to create docked windows from a given split
def create_dockable_windows(app_state: AppState) -> List[hello_imgui.DockableWindow]:
    win_search = hello_imgui.DockableWindow(
        label_="Search Window",
        dock_space_name_="MainDockSpace",
        gui_function_= lambda: show_search_window(app_state)
    )
    win_display = hello_imgui.DockableWindow(
        label_="Display Window",
        dock_space_name_="MainDockSpace",
        gui_function_= lambda: show_display_window(app_state)
    )

    dockable_windows = [
        win_search,
        win_display,
    ]

    return dockable_windows

# Helper method to layout whole dockspace
def create_default_layout(app_state: AppState) -> hello_imgui.DockingParams:
    docking_params = hello_imgui.DockingParams()
    # By default, the layout name is already "Default"
    # docking_params.layout_name = "Default"
    docking_params.docking_splits = [create_docking_split()]
    docking_params.dockable_windows = create_dockable_windows(app_state)
    return docking_params

def main():

    app_state = AppState()

    runner_params = hello_imgui.RunnerParams()
    
    # Stage 1: Define main window settings
    runner_params.app_window_params.window_title = "Recipe Database"
    runner_params.app_window_params.window_geometry.size = (1200, 900)
    runner_params.imgui_window_params.default_imgui_window_type = hello_imgui.DefaultImGuiWindowType.provide_full_screen_dock_space

    # Stage 2: Call helper methods to set up docking layout and attach to GUI functions
    runner_params.docking_params = create_default_layout(app_state)
    
    # Stage 3: Run application with all parameters
    hello_imgui.run(runner_params)


if __name__ == "__main__":
    main()
