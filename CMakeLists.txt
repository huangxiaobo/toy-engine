cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(ToyEngine LANGUAGES C CXX)
# 设置使用C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# 设置静态库输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# 设置动态库输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


# 查找当前目录及子目录中的所有 .cpp 和 .h 文件
file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.h)
 
# 将所有文件设置为源文件列表
set(SOURCE_FILES
    ${SRC_FILES}
    ${HEADER_FILES}
)

# 设置默认构建静态库
set(BUILD_SHARED_LIBS OFF)

# 使用本地3rdparty中的第三方库
# 注意: 首次使用前需要运行 ./download_deps.sh 下载依赖
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glad)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glfw)

# ImGui 没有官方CMakeLists,需要手动配置
set(imgui_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui)
add_library(imgui_lib
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PRIVATE glfw)
add_library(imgui::imgui ALIAS imgui_lib)

# Assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/assimp)

# yaml-cpp
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/yaml-cpp)

# stb (header-only库)
set(stb_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# 添加头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/engine)

# 禁用GLAD重复包含检查
add_compile_definitions(GLAD_GLAPI_EXPORT_BUILD)

find_package(glm CONFIG REQUIRED)

add_executable(toy-engine main.cpp ${SOURCE_FILES})

target_link_libraries(toy-engine PRIVATE
    glm::glm
    glad
    imgui::imgui
    glfw
    yaml-cpp::yaml-cpp
    assimp::assimp
    stb
)
