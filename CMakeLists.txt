cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(ToyEngine LANGUAGES CXX)
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
# Qt 默认动态链接，如果没有添加到系统环境变量，需要复制相关dll到相同文件夹中
set(CMAKE_PREFIX_PATH "/opt/Qt/6.8.1/gcc_arm64/lib/cmake/")

# 查找当前目录及子目录中的所有 .cpp 和 .h 文件
file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.h)
 
# 将所有文件设置为源文件列表
set(SOURCE_FILES
    ${SRC_FILES}
    ${HEADER_FILES}
)

# 添加glad
add_subdirectory(3rdparty/glad)
add_subdirectory(3rdparty/tinyxml2)
add_subdirectory(3rdparty/qtpropertybrowser)

# 添加头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/engine)
# # 添加glfw头文件
# include_directories(/opt/homebrew/Cellar/glfw/3.4/include)
# # glfw 库文件也加进来
# link_directories(/opt/homebrew/Cellar/glfw/3.4/lib)

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Widgets OpenGLWidgets)
find_package(glm CONFIG REQUIRED)
find_package(assimp)

qt_standard_project_setup()

# 将源代码文件编译为可执行文件
qt_add_executable(toy-engine
    main.cpp
    ${SOURCE_FILES}
)

target_link_libraries(toy-engine PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    glm::glm
    glad
    # glfw
    tinyxml2
    qtpropertybrowser
    assimp::assimp
)
