cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(ToyEngine LANGUAGES CXX)
# 设置使用C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# 设置静态库输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# 设置动态库输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# Qt 默认动态链接，如果没有添加到系统环境变量，需要复制相关dll到相同文件夹中
set(CMAKE_PREFIX_PATH "/opt/Qt/6.8.1/gcc_arm64/lib/cmake/")

set(SOURCES
    main.cpp
    engine/mainwindow.cpp
    engine/renderer.cpp
    engine/shader/shader.cpp
    engine/axis/axis.cpp
    engine/mesh/mesh.cpp
    engine/model/model.cpp
    engine/technique/technique.cpp
    engine/utils/utils.cpp
)

# 添加glad
add_subdirectory(3rdparty/glad)
add_subdirectory(3rdparty/tinyxml2)

# 添加glfw头文件
include_directories(/opt/homebrew/Cellar/glfw/3.4/include)
# glfw 库文件也加进来
link_directories(/opt/homebrew/Cellar/glfw/3.4/lib)

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Widgets OpenGLWidgets)
find_package(glm CONFIG REQUIRED)

qt_standard_project_setup()

qt_add_executable(toy-engine
    main.cpp
    engine/mainwindow.cpp
    engine/renderer.cpp
    engine/shader/shader.cpp
    engine/axis/axis.cpp
    engine/mesh/mesh.cpp
    engine/model/model.cpp
    engine/technique/technique.cpp
    engine/utils/utils.cpp
)

target_link_libraries(toy-engine PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    glm::glm
    glad
    glfw
    tinyxml2
)
