cmake_minimum_required(VERSION 3.10)
project(ElectronicDiary)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Include directories
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/glfw-3.4/include)

# Add ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

# Source files
set(SOURCES
    src/main.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} glfw)