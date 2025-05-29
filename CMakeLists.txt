cmake_minimum_required(VERSION 3.16)
project(ClothSimulation)

set(CMAKE_CXX_STANDARD 17)

# 设置头文件搜索路径
include_directories(
    ${PROJECT_SOURCE_DIR}/Headers
    ${PROJECT_SOURCE_DIR}/Includes
    ${PROJECT_SOURCE_DIR}/external/glad/include
    ${PROJECT_SOURCE_DIR}/external/imgui
    ${PROJECT_SOURCE_DIR}/external/imgui/backends
    ${PROJECT_SOURCE_DIR}/external/stb
    ${PROJECT_SOURCE_DIR}/external/tinyfiledialogs
    ${PROJECT_SOURCE_DIR}/external/
    /opt/homebrew/include         # 如果是 Intel Mac 改成 /usr/local/include
    /opt/homebrew/opt/libomp/include  # ✅ 添加这行！
)

# 设置库文件搜索路径
link_directories(/opt/homebrew/lib)

# 收集源文件
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    external/glad/src/glad.c
    external/imgui/*.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/tinyfiledialogs/tinyfiledialogs.c
)

# 创建可执行程序
add_executable(ClothSimulation ${SRC_FILES})


# ✅ 把可执行文件输出到项目根目录
set_target_properties(ClothSimulation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 链接系统库和 Homebrew 库
target_link_libraries(ClothSimulation
    glfw
    assimp
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    /opt/homebrew/opt/libomp/lib/libomp.dylib
)

