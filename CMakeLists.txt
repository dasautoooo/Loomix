cmake_minimum_required(VERSION 3.15)
project(Loomix)

set(CMAKE_CXX_STANDARD 20)

# Find dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(Loomix
        src/main.cpp
        third_party/imgui/backends/imgui_impl_opengl3_loader.h
        third_party/imgui/backends/imgui_impl_opengl3.h
        third_party/imgui/backends/imgui_impl_opengl3.cpp
        third_party/imgui/backends/imgui_impl_glfw.h
        third_party/imgui/backends/imgui_impl_glfw.cpp
        )

target_include_directories(Loomix PRIVATE third_party)

# Link Libraries
target_link_libraries(Loomix PRIVATE OpenGL::GL glfw imgui::imgui)

# Enable ImGui Docking
target_compile_definitions(Loomix PRIVATE IMGUI_ENABLE_DOCKING)