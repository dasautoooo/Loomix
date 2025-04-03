cmake_minimum_required(VERSION 3.15)
project(Loomix)

set(CMAKE_CXX_STANDARD 20)

# Configure cross-platform
if(WIN32)
    message(STATUS ">>> Windows build detected")
elseif(APPLE)
    message(STATUS ">>> macOS build detected")
elseif(UNIX)
    message(STATUS ">>> Linux/UNIX build detected")
endif()

# Provide typical build types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "" FORCE)

# Or define distribution option
option(DISTRIBUTION_BUILD "Enable distribution build (max optimization, no dev features)" OFF)

if(DISTRIBUTION_BUILD)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
    add_definitions(-DDISTRIBUTION_BUILD)
    message(STATUS ">>> Distribution build enabled")
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    endif()
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

if(DISTRIBUTION_BUILD)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -DNDEBUG")
endif()

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
        src/Integrators/Integrator.h
        src/Integrators/RK4Integrator.cpp
        src/Integrators/RK4Integrator.h
        src/Integrators/ExplicitEulerIntegrator.cpp
        src/Integrators/ExplicitEulerIntegrator.h
        src/Integrators/VerletIntegrator.cpp
        src/Integrators/VerletIntegrator.h
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

# Copy entire assets folder to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})