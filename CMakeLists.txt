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
add_subdirectory(3rdparty/imgui)
add_subdirectory(3rdparty/yaml-cpp)

# 添加头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/engine)
# 添加glfw头文件
include_directories(/opt/homebrew/Cellar/glfw/3.4/include)
# 包含 yaml-cpp 头文件
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/yaml-cpp/include)
# 包含 imgui 头文件
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/imgui)
# glfw 库文件也加进来
link_directories(/opt/homebrew/Cellar/glfw/3.4/lib)

# 禁用GLAD重复包含检查
add_compile_definitions(GLAD_GLAPI_EXPORT_BUILD)

find_package(glm CONFIG REQUIRED)
find_package(assimp)
find_package(glfw3 REQUIRED)

add_executable(toy-engine main.cpp ${SOURCE_FILES})

target_link_libraries(toy-engine PRIVATE
    glm::glm
    glad
    imgui::imgui
    glfw
    yaml-cpp::yaml-cpp
    assimp::assimp
)
