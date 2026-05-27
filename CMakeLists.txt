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

# 使用FetchContent自动下载第三方库
include(FetchContent)

# 设置默认构建静态库
set(BUILD_SHARED_LIBS OFF)

# 使用本地3rdparty中的glad
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glad)


# 下载glfw
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://gitcode.com/GitHub_Trending/gl/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# 下载imgui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY git@gitee.com:mirrors/imgui.git
    GIT_TAG v1.91.0
)

# 获取imgui源码并手动配置（因为官方没有提供符合需求的CMakeLists）
FetchContent_MakeAvailable(imgui)

# 创建imgui库
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

# 下载assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY git@gitee.com:mirrors/assimp.git
    GIT_TAG v5.4.2
    CMAKE_ARGS
        -DASSIMP_BUILD_TESTS=OFF
        -DASSIMP_INSTALL=OFF
        -DASSIMP_NO_EXPORT=OFF
        -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
        -DBUILD_SHARED_LIBS=OFF
)
FetchContent_MakeAvailable(assimp)

# 下载yaml-cpp
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://gitcode.com/gh_mirrors/ya/yaml-cpp.git
    GIT_TAG yaml-cpp-0.9.0
    CMAKE_ARGS
        -DYAML_CPP_BUILD_TESTS=OFF
        -DYAML_CPP_BUILD_TOOLS=OFF
        -DYAML_CPP_BUILD_CONTRIB=OFF
)
FetchContent_MakeAvailable(yaml-cpp)

# 配置stb（header-only库）
FetchContent_Declare(
    stb
    GIT_REPOSITORY git@gitee.com:mirrors/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# 为stb创建接口库（header-only）
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# 添加头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/engine)

# 禁用GLAD重复包含检查
add_compile_definitions(GLAD_GLAPI_EXPORT_BUILD)

find_package(glm CONFIG REQUIRED)

add_executable(toy-engine main.cpp ${SOURCE_FILES})

# 添加glad头文件路径
target_include_directories(toy-engine PRIVATE 
    ${glad_BINARY_DIR}/include
)

target_link_libraries(toy-engine PRIVATE
    glm::glm
    glad
    imgui::imgui
    glfw
    yaml-cpp::yaml-cpp
    assimp::assimp
    stb
)
