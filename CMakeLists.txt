cmake_minimum_required(VERSION 3.15)
project(Loomix)

set(CMAKE_CXX_STANDARD 20)

# Find dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(Loomix
        third_party/imgui/backends/imgui_impl_opengl3_loader.h
        third_party/imgui/backends/imgui_impl_opengl3.h
        third_party/imgui/backends/imgui_impl_opengl3.cpp
        third_party/imgui/backends/imgui_impl_glfw.h
        third_party/imgui/backends/imgui_impl_glfw.cpp
        third_party/glad/src/glad.c
        src/Application.cpp
        src/Application.h
        src/LoomixApp.cpp
        src/Layer.h
        src/EntryPoint.h
        src/Shader.h
        src/Shader.cpp
        src/Camera.h
        src/Camera.cpp
        src/TriangleLayer.cpp
        src/TriangleLayer.h
)

target_include_directories(Loomix PRIVATE third_party)

# Link Libraries
target_link_libraries(Loomix PRIVATE OpenGL::GL glfw imgui::imgui)

# Enable ImGui Docking
target_compile_definitions(Loomix PRIVATE IMGUI_ENABLE_DOCKING)

# Copy shaders to the build directory
file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*.*")
foreach(SHADER ${SHADER_FILES})
    configure_file(${SHADER} ${CMAKE_BINARY_DIR} COPYONLY)
endforeach()