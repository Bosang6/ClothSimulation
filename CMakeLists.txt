
cmake_minimum_required(VERSION 3.22)

project(ClothSimulation LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------
# Third-party build options
# ----------------------------

# Prefer static libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

# ----------------------------
# Third-party dependencies
# ----------------------------

add_subdirectory(external/glfw)
add_subdirectory(external/assimp)

find_package(OpenGL REQUIRED)

# ----------------------------
# Project source files
# ----------------------------

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# ----------------------------
# Application sources
# ----------------------------

add_executable(ClothSimulation
    ${PROJECT_SOURCES}

    external/glad/src/glad.c

    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp

    external/tinyfiledialogs/tinyfiledialogs.c
)

# ----------------------------
# Include directories
# ----------------------------

target_include_directories(ClothSimulation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyfiledialogs
)

# ----------------------------
# Link libraries
# ----------------------------

target_link_libraries(ClothSimulation PRIVATE
    glfw
    assimp::assimp
    OpenGL::GL
)

# ----------------------------
# Output directory
# ----------------------------

set_target_properties(ClothSimulation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
)


# Windows MSVC: use UTF-8 source encoding
if(MSVC)
    target_compile_options(ClothSimulation PRIVATE /utf-8)
endif()

