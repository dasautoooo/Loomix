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
        src/Lifecycle/Application.cpp
        src/Lifecycle/Application.h
        src/LoomixApp.cpp
        src/Layers/Layer.h
        src/Lifecycle/EntryPoint.h
        src/Utilities/Shader.h
        src/Utilities/Shader.cpp
        src/Camera.h
        src/Camera.cpp
        src/Layers/TriangleLayer.cpp
        src/Layers/TriangleLayer.h
        src/Input/Input.h
        src/Input/Input.cpp
        src/Input/KeyCodes.h
        src/Utilities/Timer.h
        src/Cloth.h
        src/Cloth.cpp
        src/Layers/ClothLayer.cpp
        src/Layers/ClothLayer.h
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