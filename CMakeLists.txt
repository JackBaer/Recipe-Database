cmake_minimum_required(VERSION 3.16)
project(recipe_app)

set(CMAKE_CXX_STANDARD 17)

# Optionally enable debug symbols
# set(CMAKE_BUILD_TYPE Debug)

# Add HelloImGui (with ImGuiBundle)
add_subdirectory(external/imgui_bundle)

configure_file(recipes.csv recipes.csv COPYONLY)

# Your sources
set(SOURCES
    main.cpp
    data.cpp
)

# Create executable
add_executable(recipe_app ${SOURCES})

# Link with imgui_bundle (this includes hello_imgui, imgui, etc.)
target_link_libraries(recipe_app PRIVATE imgui_bundle)

# Include headers from your project and imgui_bundle
target_include_directories(recipe_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui_bundle
)

