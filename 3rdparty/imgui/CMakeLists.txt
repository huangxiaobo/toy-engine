# 3rdparty/imgui/CMakeLists.txt
cmake_minimum_required(VERSION 3.26.1)

# 定义 imgui 库
add_library(imgui
        imgui.cpp
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        # 如果用到后端，可添加（例如 OpenGL3）
        backends/imgui_impl_glfw.cpp
        backends/imgui_impl_opengl3.cpp
)

# 设置头文件路径
target_include_directories(imgui PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
        /opt/homebrew/Cellar/glfw/3.4/include
)

# 查找并链接GLFW
target_link_libraries(imgui PRIVATE glfw)

# 可选：启用 demo（默认包含）
target_compile_definitions(imgui PRIVATE IMGUI_ENABLE_DEMO)

# 创建命名空间别名
add_library(imgui::imgui ALIAS imgui)