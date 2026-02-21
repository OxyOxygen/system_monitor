cmake_minimum_required(VERSION 3.15)
project(SystemMonitor VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(OpenGL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_SOURCE_DIR}/external/glfw/include
)

# ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# GLFW - We'll use prebuilt binaries or build from source
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/external/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR} EXCLUDE_FROM_ALL)

# Source files
set(SOURCES
    src/main.cpp
    src/cpu_monitor.cpp
    src/memory_monitor.cpp
    src/disk_monitor.cpp
    src/disk_io_monitor.cpp
    src/gpu_monitor.cpp
    src/power_monitor.cpp
    src/process_monitor.cpp
    src/network_monitor.cpp
    src/system_info.cpp
    src/energy_monitor.cpp
    src/ai_monitor.cpp
    src/nvml_monitor.cpp
    src/gpu_benchmark.cpp
    src/cpu_temp_monitor.cpp
    src/performance_analyzer.cpp
    src/performance_score.cpp
    src/gui.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    glfw
    pdh          # Windows Performance Data Helper
    psapi        # Process Status API
    iphlpapi     # IP Helper API (network stats)
    dxgi         # DXGI (GPU detection)
    d3d11        # Direct3D 11 (GPU benchmark)
    wbemuuid     # WMI (CPU temperature)
    ole32        # COM support
    oleaut32     # COM Automation
)

# Set as startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
