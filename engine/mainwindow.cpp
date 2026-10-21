#include <glad/gl.h>
#include "mainwindow.h"
#include "globals.h"
#include "config.h"
#include "renderer.h"
#include "model/model.h"
#include "animation/animation.h"
#include "animation/anim_target.h"
#include "light/light.h"
#include "technique/technique_light.h"
#include "material/material.h"
#include "camera/camera.h"
#include "camera/orbit_manipulator.h"
#include "terrain/terrain_manager.h"
#include "sky/sky_dome.h"
#include "particle/particle_system.h"
#include "particle/particle_emitter.h"
#include "axis/axis.h"
#include "utils/gl_state_guard.h"

// OpenGL函数由renderer.cpp提供

// Dear ImGui
#include <imgui.h>
// imgui_internal.h：DockBuilder*（DockBuilderAddNode/SplitNode/DockWindow/Finish/GetCentralNode）
// 及 ImGuiDockNode 结构体（中央节点 Pos/Size）定义所在，仅在 mainwindow.cpp 使用
#include <imgui_internal.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

#include <iostream>

// stb_image_write：截图功能用于将 glReadPixels 读回的像素保存为 PNG 文件。
// 定义 STB_IMAGE_WRITE_IMPLEMENTATION 生成实现（头文件仅此一处定义，
// 若其他翻译单元再次包含需保持仅有本次定义，避免重复链接错误）。
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// 截图文件名时间戳格式化
#include <ctime>
#include <chrono>
#include <glm/gtc/type_ptr.hpp>

// GLFW错误回调
void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// FPS 曲线采样容量：每 1/15 秒一个采样点 × 600 = 40 秒历史
static constexpr size_t kFpsHistoryCapacity = 600;
// FPS 曲线采样间隔（秒）：按此周期结算一次窗口平均帧率（15 次/秒）
static constexpr float kFpsSampleIntervalSec = 1.0f / 15.0f;

// 底部状态栏高度：单行文本垂直居中
static constexpr float kStatusBarHeight = 26.0f;

// ---- GLFW 输入回调（ImGui 优先模式） ----
// 所有回调遵循同一模式：先转发给 ImGui → 检查 WantCapture → 再交给引擎
// 通过 glfwSetWindowUserPointer 将 GLFWwindow 关联到 ToyEngineMainWindow 实例

void ToyEngineMainWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // 先让 ImGui 处理（更新 WantCaptureMouse 状态）
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    // ImGui 正在捕获鼠标时，不转发给引擎
    if (ImGui::GetIO().WantCaptureMouse) return;

    // 从 window user pointer 获取实例
    auto* self = static_cast<ToyEngineMainWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            self->OnMouseLeftButtonDown();
            self->m_mouse_left_pressed = true;
        } else {
            self->OnMouseLeftButtonUp();
            self->m_mouse_left_pressed = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            self->OnMouseRightButtonDown();
            self->m_mouse_right_pressed = true;
        } else {
            self->OnMouseRightButtonUp();
            self->m_mouse_right_pressed = false;
        }
    }
}

void ToyEngineMainWindow::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // 先让 ImGui 处理（更新 WantCaptureMouse 状态）
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

    auto* self = static_cast<ToyEngineMainWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    // 计算并更新鼠标增量基线
    double deltaX = xpos - self->m_current_mouse_x;
    double deltaY = ypos - self->m_current_mouse_y;
    self->m_current_mouse_x = xpos;
    self->m_current_mouse_y = ypos;

    // ImGui 正在捕获鼠标时，不驱动相机，并重置相机交互状态避免残留
    if (ImGui::GetIO().WantCaptureMouse) {
        self->m_camera_panning = false;
        return;
    }

    if (self->m_mouse_left_pressed || self->m_mouse_right_pressed) {
        self->OnMouseMove(deltaX, deltaY);
    }
}

void ToyEngineMainWindow::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // 先让 ImGui 处理
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    if (ImGui::GetIO().WantCaptureMouse) return;

    auto* self = static_cast<ToyEngineMainWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    self->OnMouseWheel(yoffset);
}

void ToyEngineMainWindow::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // 先让 ImGui 处理（更新 WantCaptureKeyboard 状态）
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    if (ImGui::GetIO().WantCaptureKeyboard) return;

    auto* self = static_cast<ToyEngineMainWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // 仅在按下瞬间响应，避免按住重复触发
    if (action != GLFW_PRESS) {
        return;
    }

    // F12：截图，与「工具 → 截图」菜单项行为一致
    if (key == GLFW_KEY_F12) {
        self->m_screenshot_pending = true;
        return;
    }

    // Ctrl+1~7：切换各面板显示，与「视图」菜单中的快捷键标注一一对应
    if ((mods & GLFW_MOD_CONTROL) != 0) {
        switch (key) {
            case GLFW_KEY_1: self->m_show_resource_list = !self->m_show_resource_list; break;
            case GLFW_KEY_2: self->m_show_properties = !self->m_show_properties; break;
            case GLFW_KEY_3: self->m_show_shadow_properties = !self->m_show_shadow_properties; break;
            case GLFW_KEY_4: self->m_show_post_process = !self->m_show_post_process; break;
            case GLFW_KEY_5: self->m_show_debug_properties = !self->m_show_debug_properties; break;
            case GLFW_KEY_6: self->m_show_fps_graph = !self->m_show_fps_graph; break;
            case GLFW_KEY_7: self->m_show_shadow_depth_map = !self->m_show_shadow_depth_map; break;
            default: break;
        }
    }
}

// 构造/析构定义在 .cpp：unique_ptr 成员需持有类型完整
ToyEngineMainWindow::ToyEngineMainWindow() = default;

ToyEngineMainWindow::~ToyEngineMainWindow() {
    Cleanup();
}

bool ToyEngineMainWindow::Initialize() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 从配置文件读取窗口尺寸
    try {
        gConfig = Config::LoadFromYaml("./resource/world.yaml");
        m_window_width = gConfig->m_window.m_window_width;
        m_window_height = gConfig->m_window.m_window_height;
    } catch (...) {
        m_window_width = 1280;
        m_window_height = 720;
        std::cerr << "Warning: Failed to load config file, using default window size" << std::endl;
    }

    m_window = glfwCreateWindow(m_window_width, m_window_height, "Toy Engine", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(m_window, this);

    // 初始化鼠标位置基线，供 CursorPosCallback 计算增量
    glfwGetCursorPos(m_window, &m_current_mouse_x, &m_current_mouse_y);

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // 启用 Docking：DockSpace/DockBuilder 布局（imgui-docking 分支特性）依赖此标志
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 加载中文字体
    std::cout << "正在加载中文字体..." << std::endl;
    FILE* fontFile = fopen("./resource/font/微软雅黑.ttf", "rb");
    if (fontFile) {
        fclose(fontFile);
        std::cout << "找到字体文件" << std::endl;
        ImFont* chineseFont = io.Fonts->AddFontFromFileTTF(
            "./resource/font/微软雅黑.ttf", 16.0f, nullptr,
            io.Fonts->GetGlyphRangesChineseFull());
        if (chineseFont) {
            std::cout << "✅ 成功加载中文字体" << std::endl;
            io.FontDefault = chineseFont;
        } else {
            std::cerr << "❌ 加载中文字体失败" << std::endl;
        }
    } else {
        std::cerr << "❌ 找不到字体文件: ./resource/font/微软雅黑.ttf" << std::endl;
    }

    ImGui::StyleColorsDark();
    // 默认暗色主题的间距/圆角微调：收紧控件圆角、略增内边距，面板信息密度更均匀
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 4.0f;
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 注册 GLFW 输入回调（ImGui 优先模式），替代原来的轮询方式
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetCursorPosCallback(m_window, CursorPosCallback);
    glfwSetScrollCallback(m_window, ScrollCallback);
    glfwSetKeyCallback(m_window, KeyCallback);

    // 初始化渲染器（unique_ptr 自管，析构自动释放）
    m_renderer = std::make_unique<Renderer>();
    m_renderer->init(m_window_width, m_window_height);

    m_last_time = static_cast<float>(glfwGetTime());
    return true;
}

void ToyEngineMainWindow::Run() {
    while (!glfwWindowShouldClose(m_window)) {
        ProcessInput();
        RenderFrame();
        glfwPollEvents();
    }
}

void ToyEngineMainWindow::ProcessInput() {
    // 输入处理已迁移到 GLFW 回调（MouseButtonCallback / CursorPosCallback / ScrollCallback / KeyCallback），
    // 见上方实现的 ImGui 优先模式。此处保留空实现以维持调用约定。
}

/*
 * 渲染一帧
 *
 * 布局策略（DockSpace 版本）：
 *   1. 先启动 ImGui 帧并渲染 DockSpace，拿到中央节点（3D 渲染视口）在
 *      ImGui 逻辑坐标下的矩形（左上原点、Y 向下）；
 *   2. 将该矩形换算为 framebuffer 像素（含 Retina 像素/点缩放与 Y 轴翻转），
 *      设置为 OpenGL 视口并绘制 3D 场景；
 *   3. 恢复全窗口视口让 ImGui 面板（停靠于左右节点）覆盖其上。
 */
void ToyEngineMainWindow::RenderFrame() {
    float currentTime = static_cast<float>(glfwGetTime());
    m_delta_time = currentTime - m_last_time;
    m_last_time = currentTime;

    // FPS 曲线采样：按 kFpsSampleIntervalSec（1/15 秒）结算一次窗口平均帧率
    // （帧数/流逝时间），比逐帧瞬时值平滑、又比低频采样更灵敏；
    // 结算后清零累加器进入下一个窗口
    if (m_delta_time > 0.0f) {
        m_fps_sample_frames++;
        m_fps_sample_elapsed += m_delta_time;
        if (m_fps_sample_elapsed >= kFpsSampleIntervalSec) {
            if (m_fps_history.size() >= kFpsHistoryCapacity) {
                m_fps_history.erase(m_fps_history.begin());
            }
            m_fps_history.push_back(static_cast<float>(m_fps_sample_frames) / m_fps_sample_elapsed);
            m_fps_sample_frames = 0;
            m_fps_sample_elapsed = 0.0f;
        }
    }

    m_renderer->update(static_cast<long long>(m_delta_time * 1000));

    // 获取实际 framebuffer 像素尺寸（Retina 下通常为窗口 points 尺寸的 2 倍）。
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    // ---- ImGui 帧先行：DockSpace 完成布局后才能查询中央节点矩形 ----
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 创建/渲染 DockSpace，并将中央节点矩形写入 m_viewport_x/Y/Width/Height
    CreateDockSpace();

    // 将中央节点矩形从 ImGui 逻辑坐标（左上原点、Y 向下）换算为
    // framebuffer 像素坐标（左下原点、Y 向上）：像素 = 逻辑值 × 缩放比
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    const float xScale = (displaySize.x > 0.0f) ? static_cast<float>(fbWidth) / displaySize.x : 1.0f;
    const float yScale = (displaySize.y > 0.0f) ? static_cast<float>(fbHeight) / displaySize.y : 1.0f;

    const int viewportX = static_cast<int>(m_viewport_x * xScale);
    const int viewportY = static_cast<int>(static_cast<float>(fbHeight) - (m_viewport_y + m_viewport_height) * yScale);
    const int viewportW = static_cast<int>(m_viewport_width * xScale);
    const int viewportH = static_cast<int>(m_viewport_height * yScale);

    // 3D 绘制期间把视口切到中央节点；guard 在离开作用域时恢复进入前的实际视口，
    // 而不是硬编码全窗口尺寸
    {
        GLViewportGuard scene_viewport;
        // 中央节点尺寸异常（首帧布局尚未完成）时回退到全窗口视口
        if (viewportW <= 0 || viewportH <= 0) {
            m_renderer->resize(fbWidth, fbHeight);
            glViewport(0, 0, fbWidth, fbHeight);
        } else {
            m_renderer->resize(viewportW, viewportH);
            glViewport(viewportX, viewportY, viewportW, viewportH);
        }
        m_renderer->draw(static_cast<long long>(m_delta_time * 1000));
    }

    // 绘制面板（资源列表/属性面板停靠于 DockSpace 左右节点）
    CreateUI();

    // 在视口左下角叠加屏幕空间坐标轴 gizmo：
    // 位于 CreateUI() 之后、ImGui::Render() 之前，确保背景绘制列表已建立；
    // 用相机视图矩阵驱动三轴朝向（随相机旋转变化）
    DrawViewportAxisGizmo();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 截图请求处理：等待本帧完整绘制（场景+后处理+ImGui+轴gizmo）之后、
    // 缓冲交换之前执行，确保捕获的是用户看到的完整一帧
    if (m_screenshot_pending) {
        m_screenshot_pending = false;
        SaveScreenshot();
    }

    glfwSwapBuffers(m_window);
}

/*
 * 创建/渲染 DockSpace 并建立初始三栏布局
 *
 * 布局：DockBuilder 将 DockSpace 竖直分为左/中/右三栏——
 *   - 左节点：资源列表（占总宽 20%），下方为调试属性
 *   - 右节点：属性面板（占总宽 20%），下方为阴影/后处理/深度贴图 tab
 *   - 中央节点：留空，作为 3D 渲染视口（PassthruCentralNode 模式，输入穿透用于相机控制）
 *
 * 菜单栏占据顶部，底部保留状态栏高度；FPS 曲线不预建停靠节点，
 * 需要时以浮动窗出现在视口底部（隐藏时不占用视口空间）。
 * 仅首帧执行 DockBuilder 初始化；此后 DockSpace 完全接管布局，用户可自由
 * 拖拽/浮动/调整各面板。每帧结束时将中央节点矩形记录到 m_viewport* 成员，
 * 供 RenderFrame 换算 glViewport 使用。
 */
void ToyEngineMainWindow::CreateDockSpace() {
    ImGuiID dockspaceId = ImGui::GetID("ToyEngineDockSpace");

    // 首次运行时用 DockBuilder 建立初始布局（之后保留用户调整结果）
    if (!m_dock_space_initialized) {
        m_dock_space_initialized = true;

        // 清空可能的残留节点，重新创建 DockSpace 根节点
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

        // 左右栏各占总宽 20%：先分出左栏，再从剩余 80% 中按 0.25 分出右栏（= 20% / 80%）
        ImGuiID mainNode = dockspaceId;
        ImGuiID leftNode = ImGui::DockBuilderSplitNode(
            mainNode, ImGuiDir_Left, 0.20f, nullptr, &mainNode);
        ImGuiID rightNode = ImGui::DockBuilderSplitNode(
            mainNode, ImGuiDir_Right, 0.25f, nullptr, &mainNode);

        // 面板窗口按标题绑定到对应节点（窗口标题必须与停靠目标一致）
        // 调试属性面板停靠于左节点下方（与资源列表同栏，垂直排列）
        ImGuiID leftBottomNode = ImGui::DockBuilderSplitNode(
            leftNode, ImGuiDir_Down, 0.30f, nullptr, &leftNode);
        // 阴影属性面板停靠于右节点下方（与属性面板同栏，垂直排列）
        ImGuiID rightBottomNode = ImGui::DockBuilderSplitNode(
            rightNode, ImGuiDir_Down, 0.35f, nullptr, &rightNode);
        ImGui::DockBuilderDockWindow("资源列表", leftNode);
        ImGui::DockBuilderDockWindow("调试属性", leftBottomNode);
        // 右节点停靠四个窗口：模型被选中时显示 模型/材质/动画（tab 切换），
        // 其它类型或无选中时显示 属性；未渲染的窗口隐藏，但保留停靠位置
        ImGui::DockBuilderDockWindow("属性", rightNode);
        ImGui::DockBuilderDockWindow("模型", rightNode);
        ImGui::DockBuilderDockWindow("材质", rightNode);
        ImGui::DockBuilderDockWindow("动画", rightNode);
        ImGui::DockBuilderDockWindow("阴影属性", rightBottomNode);
        ImGui::DockBuilderDockWindow("后处理", rightBottomNode);
        // 深度贴图作为右下的第三个 tab：打开后不再以浮动窗遮住视口
        ImGui::DockBuilderDockWindow("阴影深度贴图", rightBottomNode);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    // 全屏宿主窗口承载菜单栏 + DockSpace + 状态栏：无标题、无边框
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    // PassthruCentralNode 模式要求宿主窗口背景透明，否则中央节点会被刷上
    // ImGuiCol_WindowBg，遮挡 3D 场景
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("##DockSpaceHost", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar(3);

    // 主菜单栏：必须位于 DockSpace 之前渲染，
    // DockSpace 的可用区域会自动扣除菜单栏高度
    CreateMenuBar();

    // 渲染 DockSpace：显式扣掉底部状态栏高度（空节点不会自动折叠，
    // 所以不预留任何隐藏面板的分割槽位）；PassthruCentralNode = 中央节点
    // 保持为空且输入穿透，让 GLFW 回调（相机控制）在中央区域可正常收到鼠标事件
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::DockSpace(dockspaceId,
        ImVec2(avail.x, ImMax(avail.y - kStatusBarHeight, 1.0f)),
        ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::SetCursorPos(ImVec2(0.0f, ImGui::GetWindowSize().y - kStatusBarHeight));
    ShowStatusBar();
    ImGui::End();

    // 记录中央节点矩形（ImGui 逻辑坐标，左上原点、Y 向下），供 RenderFrame 换算
    ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspaceId);
    if (centralNode) {
        m_viewport_x = centralNode->Pos.x;
        m_viewport_y = centralNode->Pos.y;
        m_viewport_width = centralNode->Size.x;
        m_viewport_height = centralNode->Size.y;
    }
}

/*
 * 主菜单栏（视图 / 工具 / 帮助）
 *
 * 菜单结构：
 *   - 视图：按用途分组的面板开关（快捷键 Ctrl+1~7，已在 KeyCallback 中实现），
 *     每组带灰色小标题；「模型窗口」子菜单收纳模型/材质/动画三个右栏 tab；
 *     末尾「重置布局」恢复默认三栏，「清除选择」取消当前选中对象
 *   - 工具：截图（F12）、光源调试线框快捷开关
 *   - 帮助：关于弹窗（含快捷键速查）
 *   - 退出：请求关闭窗口（与 ESC 键行为一致，ESC 在 KeyCallback 中处理），
 *     置于菜单栏末尾符合通用软件惯例
 *
 * 必须在宿主窗口 Begin() 之后、DockSpace() 之前调用，
 * 这样 DockSpace 可用区域自动扣除菜单栏高度，中央节点随之正确下移。
 */
void ToyEngineMainWindow::CreateMenuBar() {
    if (ImGui::BeginMenuBar()) {
        // 视图：分组收纳所有可开关面板；MenuItem 第三参数为选中状态，
        // 点击时自动切换并显示勾选标记，与面板可见性绑定
        if (ImGui::BeginMenu("视图")) {
            ImGui::TextDisabled("基础面板");
            ImGui::MenuItem("资源列表", "Ctrl+1", &m_show_resource_list);
            ImGui::MenuItem("属性", "Ctrl+2", &m_show_properties);

            ImGui::Separator();
            ImGui::TextDisabled("渲染调试");
            ImGui::MenuItem("阴影属性", "Ctrl+3", &m_show_shadow_properties);
            ImGui::MenuItem("后处理", "Ctrl+4", &m_show_post_process);
            ImGui::MenuItem("调试属性", "Ctrl+5", &m_show_debug_properties);
            ImGui::MenuItem("FPS曲线", "Ctrl+6", &m_show_fps_graph);

            ImGui::Separator();
            ImGui::TextDisabled("深度调试");
            ImGui::MenuItem("阴影深度贴图", "Ctrl+7", &m_show_shadow_depth_map);

            // 模型选中时右栏拆分出的三个 tab，独立开关（受「属性」总开关控制显示）
            ImGui::Separator();
            ImGui::TextDisabled("模型窗口");
            ImGui::MenuItem("模型", nullptr, &m_show_model_window);
            ImGui::MenuItem("材质", nullptr, &m_show_material_window);
            ImGui::MenuItem("动画", nullptr, &m_show_animation_window);

            // 重置布局：标记 DockSpace 待重建，下一帧 DockBuilder 恢复默认三栏；
            // 同时把面板可见性恢复为启动默认值
            ImGui::Separator();
            if (ImGui::MenuItem("重置布局")) {
                ResetLayout();
            }

            // 清除选择：取消当前选中对象，属性面板回到空状态引导提示
            if (ImGui::MenuItem("清除选择")) {
                ClearSelection();
                m_renderer->ClearPickHighlight();
            }
            ImGui::EndMenu();
        }

        // 工具：收纳编辑器辅助功能；「截图」点击仅置位请求标志，
        // 实际读取延迟到本帧渲染完成（RenderFrame 末尾）执行，
        // 确保截取到包含全部 UI 叠加的完整一帧
        if (ImGui::BeginMenu("工具")) {
            if (ImGui::MenuItem("截图", "F12")) {
                m_screenshot_pending = true;
            }
            // 光源 gizmo 显示开关，与调试属性面板中的同名选项绑定同一状态
            bool debugDraw = m_renderer->IsDebugDrawEnabled();
            if (ImGui::MenuItem("光源调试线框", nullptr, debugDraw)) {
                m_renderer->SetDebugDrawEnabled(!debugDraw);
            }
            ImGui::EndMenu();
        }

        // 帮助：关于弹窗
        if (ImGui::BeginMenu("帮助")) {
            if (ImGui::MenuItem("关于 Toy Engine")) {
                ImGui::OpenPopup("关于 Toy Engine");
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        // 退出：点击后设置关闭标志，主循环在下一帧退出
        if (ImGui::MenuItem("退出", "Esc")) {
            glfwSetWindowShouldClose(m_window, true);
        }

        ImGui::EndMenuBar();

        // 关于弹窗：与菜单栏同属宿主窗口的 ID 作用域，故在 EndMenuBar 之后、
        // 宿主窗口 End 之前渲染
        if (ImGui::BeginPopupModal("关于 Toy Engine", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Toy Engine 0.1");
            ImGui::Text("基于 OpenGL 3.3 + Dear ImGui (docking)");
            ImGui::Separator();

            // 快捷键速查：集中说明常用操作，避免用户记不住键位
            ImGui::TextDisabled("快捷键");
            ImGui::Text("左键拖动      旋转视角");
            ImGui::Text("右键拖动      平移视角");
            ImGui::Text("滚轮          缩放视角");
            ImGui::Text("Ctrl+1~7      开关面板");
            ImGui::Text("F12           截图");
            ImGui::Text("Esc           退出");
            ImGui::Separator();

            if (ImGui::Button("关闭")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

/*
 * 恢复默认布局与面板可见性
 *
 * 菜单「视图 → 重置布局」调用：标记 DockSpace 待重建
 * （下一帧 DockBuilder 重新三栏），并复位全部面板开关。
 */
void ToyEngineMainWindow::ResetLayout() {
    m_dock_space_initialized = false;
    m_show_resource_list = true;
    m_show_properties = true;
    m_show_shadow_properties = true;
    m_show_post_process = true;
    m_show_debug_properties = true;
    m_show_fps_graph = false;
    m_show_shadow_depth_map = false;
    m_show_model_window = true;
    m_show_material_window = true;
    m_show_animation_window = true;
}

/*
 * 底部状态栏
 *
 * 常驻显示帧率、选中对象与视口分辨率：面板隐藏后仍能确认引擎状态。
 */
void ToyEngineMainWindow::ShowStatusBar() {
    const ImVec2 origin = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const float top = size.y - kStatusBarHeight;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(ImVec2(origin.x, origin.y + top),
                             ImVec2(origin.x + size.x, origin.y + size.y),
                             ImGui::GetColorU32(ImGuiCol_MenuBarBg));
    draw_list->AddLine(ImVec2(origin.x, origin.y + top),
                       ImVec2(origin.x + size.x, origin.y + top),
                       ImGui::GetColorU32(ImGuiCol_Separator));

    ImGui::SetCursorPos(ImVec2(8.0f, top + (kStatusBarHeight - ImGui::GetTextLineHeight()) * 0.5f));

    // 帧率取最近的窗口平均采样（与 FPS 曲线面板同源），历史为空时显示占位
    if (!m_fps_history.empty()) {
        const float cur_fps = m_fps_history.back();
        ImGui::Text("FPS %.1f (%.1f ms)", cur_fps, cur_fps > 0.0f ? 1000.0f / cur_fps : 0.0f);
    } else {
        ImGui::Text("FPS --");
    }

    std::string selection = "选中: --";
    if (m_selected_object != nullptr) {
        std::string type_label;
        std::string name;
        if (m_selected_object_type == "Model") {
            type_label = "模型";
            name = static_cast<Model *>(m_selected_object)->GetName();
        } else if (m_selected_object_type == "Light") {
            type_label = "灯光";
            name = static_cast<Light *>(m_selected_object)->GetName();
        } else if (m_selected_object_type == "Camera") {
            type_label = "摄像机";
            name = static_cast<Camera *>(m_selected_object)->GetName();
        } else if (m_selected_object_type == "Terrain") {
            type_label = "地形";
        } else if (m_selected_object_type == "SkyDome") {
            type_label = "天空穹";
        } else if (m_selected_object_type == "Particle") {
            type_label = "粒子";
            name = std::to_string(m_selected_particle_index);
        }
        selection = "选中: " + type_label + (name.empty() ? "" : " · " + name);
    }
    ImGui::SameLine(0.0f, 24.0f);
    ImGui::TextUnformatted(selection.c_str());

    char resolution[64];
    std::snprintf(resolution, sizeof(resolution), "视口 %d × %d",
                  static_cast<int>(m_viewport_width), static_cast<int>(m_viewport_height));
    const float resolution_width = ImGui::CalcTextSize(resolution).x;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - resolution_width - 8.0f);
    ImGui::TextUnformatted(resolution);
}

/*
 * 创建整个ImGui界面
 *
 * DockSpace 版本：布局由 DockSpace 统一管理（初始三栏已在 CreateDockSpace()
 * 中通过 DockBuilder 建立），此处仅负责绘制停靠的面板窗口，不再手动设置
 * 位置/尺寸，也不再使用自绘分隔条。
 */
void ToyEngineMainWindow::CreateUI() {
    // 资源列表面板：停靠于左节点
    if (m_show_resource_list) {
        CreateResourceListPanel();
    }

    // 右栏面板：模型属性拆为 模型/材质/动画 三个独立窗口（同节点 tab 切换）；
    // 灯光选中时显示 属性 + 动画 两个窗口；其它类型或无选中时仍用单一「属性」窗口
    if (m_show_properties) {
        if (m_selected_object != nullptr && m_selected_object_type == "Model") {
            CreateModelWindow();
            CreateMaterialWindow();
            CreateAnimationWindow();
        } else if (m_selected_object != nullptr && m_selected_object_type == "Light") {
            CreatePropertiesPanel();
            CreateAnimationWindow();
        } else {
            CreatePropertiesPanel();
        }
    }

    // 阴影属性面板：停靠于右节点（属性面板下方）
    if (m_show_shadow_properties) {
        ShowShadowPropertiesPanel();
    }

    // 后处理面板：与阴影属性同栏（右下节点）tab 切换
    if (m_show_post_process) {
        ShowPostProcessPanel();
    }

    // 调试属性面板：停靠于左节点（资源列表下方），承载渲染调试开关
    if (m_show_debug_properties) {
        ShowDebugPropertiesPanel();
    }

    // 停靠于视口底部的 FPS 曲线面板（15 次/秒的窗口平均帧率折线）
    if (m_show_fps_graph) {
        ShowFpsGraph();
    }

    // 阴影深度贴图可视化调试面板（把深度图作为纹理显示）
    if (m_show_shadow_depth_map) {
        ShowShadowDepthMapPanel();
    }
}

/*
 * 左栏：资源列表面板
 *
 * 显示 world.yaml 中定义的所有资源，按类型分组：
 *   - 摄像机（可切换当前摄像机，正在使用的标「当前」）
 *   - 灯光（可选中编辑属性）
 *   - 模型（可选中编辑属性）
 *   - 场景（地形、天空穹两个单例归入同一分组）
 *   - 粒子系统（可选中编辑发射器参数）
 */
void ToyEngineMainWindow::CreateResourceListPanel() {
    // 停靠于 DockSpace 左节点：不能带 NoMove/NoResize，否则无法被 DockBuilder 停靠
    // 与用户拖拽调整
    ImGui::Begin("资源列表", &m_show_resource_list,
        ImGuiWindowFlags_NoCollapse);

    // ---- 摄像机 ----
    const auto& cameras = m_renderer->GetCameras();
    if (ImGui::TreeNodeEx(("摄像机 (" + std::to_string(cameras.size()) + ")").c_str(),
                          ImGuiTreeNodeFlags_DefaultOpen)) {
        for (size_t i = 0; i < cameras.size(); ++i) {
            const auto& camera = cameras[i];
            std::string displayName = camera->GetName().empty()
                ? "Camera " + std::to_string(i) : camera->GetName();
            // 正在渲染使用的摄像机加标记，与「选中高亮」区分
            if (m_current_camera_index == static_cast<int>(i)) {
                displayName += "（当前）";
            }
            bool isSelected = (m_selected_object == camera.get()
                && m_selected_object_type == "Camera"
                && m_current_camera_index == static_cast<int>(i));

            if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                m_renderer->SwitchCamera(static_cast<int>(i));
                m_current_camera_index = static_cast<int>(i);
                SelectObject(m_renderer->GetCamera(), "Camera");
            }
        }
        ImGui::TreePop();
    }

    // ---- 灯光 ----
    const auto& lights = m_renderer->GetLights();
    if (ImGui::TreeNodeEx(("灯光 (" + std::to_string(lights.size()) + ")").c_str(),
                          ImGuiTreeNodeFlags_DefaultOpen)) {
        for (size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];
            if (light == nullptr) continue;
            std::string nodeName = light->GetName() + "##light" + std::to_string(i);
            bool isSelected = (m_selected_object == light.get() && m_selected_object_type == "Light");
            if (ImGui::Selectable(nodeName.c_str(), isSelected)) {
                SelectObject(light.get(), "Light");
            }
        }
        ImGui::TreePop();
    }

    // ---- 模型 ----
    const auto& models = m_renderer->GetModels();
    if (ImGui::TreeNodeEx(("模型 (" + std::to_string(models.size()) + ")").c_str(),
                          ImGuiTreeNodeFlags_DefaultOpen)) {
        for (size_t i = 0; i < models.size(); ++i) {
            const auto& model = models[i];
            if (model == nullptr) continue;
            std::string nodeName = model->GetName() + "##model" + std::to_string(i);
            bool isSelected = (m_selected_object == model.get() && m_selected_object_type == "Model");
            if (ImGui::Selectable(nodeName.c_str(), isSelected)) {
                SelectObject(model.get(), "Model");
            }
        }
        ImGui::TreePop();
    }

    // ---- 场景（地形、天空穹两个单例，归入同一分组与其它资源风格一致）----
    TerrainManager *terrainMgr = m_renderer->GetTerrainManager();
    SkyDome *sky = m_renderer->GetSkyDome();
    if (terrainMgr != nullptr || sky != nullptr) {
        const std::string sceneGroup = "场景 (" + std::to_string(
            (terrainMgr != nullptr ? 1 : 0) + (sky != nullptr ? 1 : 0)) + ")";
        if (ImGui::TreeNodeEx(sceneGroup.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (terrainMgr != nullptr) {
                bool isSelected = (m_selected_object_type == "Terrain");
                if (ImGui::Selectable("地形", isSelected)) {
                    SelectObject(terrainMgr, "Terrain");
                }
            }
            if (sky != nullptr) {
                bool isSelected = (m_selected_object_type == "SkyDome");
                if (ImGui::Selectable("天空穹", isSelected)) {
                    SelectObject(sky, "SkyDome");
                }
            }
            ImGui::TreePop();
        }
    }

    // ---- 粒子系统 ----
    const auto& particles = m_renderer->GetParticleSystems();
    if (!particles.empty()) {
        if (ImGui::TreeNodeEx(("粒子系统 (" + std::to_string(particles.size()) + ")").c_str(),
                              ImGuiTreeNodeFlags_DefaultOpen)) {
            for (size_t i = 0; i < particles.size(); ++i) {
                auto emitter = particles[i]->GetEmitter();
                if (emitter == nullptr) continue;
                // 显示名带发射器位置坐标，便于区分多个粒子系统
                char displayName[64];
                std::snprintf(displayName, sizeof(displayName),
                              "粒子 %d  (%.1f, %.1f, %.1f)##particle%d",
                              static_cast<int>(i),
                              emitter->m_position.x, emitter->m_position.y, emitter->m_position.z,
                              static_cast<int>(i));
                bool isSelected = (m_selected_object_type == "Particle"
                    && m_selected_particle_index == static_cast<int>(i));
                if (ImGui::Selectable(displayName, isSelected)) {
                    SelectObject(particles[i].get(), "Particle");
                    m_selected_particle_index = static_cast<int>(i);
                }
            }
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

/*
 * 右栏：属性面板
 *
 * 根据当前选中的资源类型，显示对应的属性编辑器。
 * 每种资源类型都有专门的 Show*Properties() 方法，
 * 确保所有可编辑属性都能在面板中展示和修改。
 */
void ToyEngineMainWindow::CreatePropertiesPanel() {
    // 停靠于 DockSpace 右节点：与资源列表面板同理，不设 NoMove/NoResize
    ImGui::Begin("属性", &m_show_properties,
        ImGuiWindowFlags_NoCollapse);

    if (m_selected_object == nullptr) {
        // 空状态：提示居中显示，引导用户如何选择资源
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y * 0.35f);
        const char *hint1 = "未选中任何资源";
        const char *hint2 = "点击左侧资源列表，或在视口中点击对象";
        const float width = ImGui::CalcTextSize(hint2).x;
        const float startX = ImGui::GetCursorPosX()
            + (ImGui::GetContentRegionAvail().x - width) * 0.5f;
        ImGui::SetCursorPosX(startX);
        ImGui::TextDisabled("%s", hint1);
        ImGui::SetCursorPosX(startX);
        ImGui::TextDisabled("%s", hint2);
        ImGui::End();
        return;
    }

    if (m_selected_object_type == "Light") {
        ShowLightProperties();
    } else if (m_selected_object_type == "Camera") {
        ShowCameraProperties();
    } else if (m_selected_object_type == "Terrain") {
        ShowTerrainProperties();
    } else if (m_selected_object_type == "SkyDome") {
        ShowSkyDomeProperties();
    } else if (m_selected_object_type == "Particle") {
        ShowParticleProperties();
    }

    ImGui::End();
}

// ---- 模型窗口内容（右栏「模型」tab）----
// 只读信息（类型/名称）+ 变换属性（位置/缩放/旋转）；
// 材质与动画分别独立成窗口，见 CreateMaterialWindow / CreateAnimationWindow
void ToyEngineMainWindow::ShowModelProperties() {
    Model* model = static_cast<Model*>(m_selected_object);

    ImGui::SeparatorText("基本信息");
    ImGui::Text("类型: 模型");
    ImGui::Text("名称: %s", model->GetName().c_str());

    ImGui::SeparatorText("变换");
    glm::vec3 position = model->GetPosition();
    if (ImGui::DragFloat3("位置", glm::value_ptr(position), 0.1f)) {
        model->SetTranslate(position);
    }

    glm::vec3 scale = model->GetScale();
    if (ImGui::DragFloat3("缩放", glm::value_ptr(scale), 0.1f)) {
        model->SetScale(scale);
    }

    float rotX = model->GetRotationX();
    float rotY = model->GetRotationY();
    float rotZ = model->GetRotationZ();
    bool rotChanged = false;
    rotChanged |= ImGui::DragFloat("旋转 X (度)", &rotX, 1.0f);
    rotChanged |= ImGui::DragFloat("旋转 Y (度)", &rotY, 1.0f);
    rotChanged |= ImGui::DragFloat("旋转 Z (度)", &rotZ, 1.0f);
    if (rotChanged) {
        model->SetRotation(rotX, rotY, rotZ);
    }
}

// ---- 模型窗口：类型/名称 + 变换 ----
void ToyEngineMainWindow::CreateModelWindow() {
    if (m_selected_object == nullptr || m_selected_object_type != "Model") {
        return;
    }
    // 选中模型后首个帧把本窗口设为当前活动 tab（SetNextWindowFocus 作用于下个 Begin）
    if (m_focus_model_window) {
        ImGui::SetNextWindowFocus();
        m_focus_model_window = false;
    }
    ImGui::Begin("模型", &m_show_model_window, ImGuiWindowFlags_NoCollapse);
    ShowModelProperties();
    ImGui::End();
}

// ---- 材质窗口：渲染风格 + 材质参数 ----
void ToyEngineMainWindow::CreateMaterialWindow() {
    if (m_selected_object == nullptr || m_selected_object_type != "Model") {
        return;
    }
    ImGui::Begin("材质", &m_show_material_window, ImGuiWindowFlags_NoCollapse);
    ShowModelMaterialProperties(static_cast<Model*>(m_selected_object));
    ImGui::End();
}

// ---- 动画窗口：绑定到当前选中模型/灯光的动画展示 ----
void ToyEngineMainWindow::CreateAnimationWindow() {
    if (m_selected_object == nullptr) {
        return;
    }
    if (m_selected_object_type != "Model" && m_selected_object_type != "Light") {
        return;
    }
    ImGui::Begin("动画", &m_show_animation_window, ImGuiWindowFlags_NoCollapse);
    // 按实际类型转换后隐式上转 IAnimTarget（void* 直接静态转换到子类偏移量不可靠）
    IAnimTarget *target = nullptr;
    if (m_selected_object_type == "Model") {
        target = static_cast<Model*>(m_selected_object);
    } else {
        target = static_cast<Light*>(m_selected_object);
    }
    ShowAnimationProperties(target);
    ImGui::End();
}

/* 
 * 动画窗口内容
 *
 * 遍历渲染器登记的全部动画，筛选绑定到当前目标（模型或灯光）的部分；
 * 每个动画节点默认展开：显示类型摘要、启用开关，以及各已配置通道
 * （位移/缩放/旋转/颜色/强度）的参数编辑控件（实时写回动画通道）。
 * 动画源数据来自 world.yaml animations 段，运行时由 Renderer 统一推进。
 */

// 由动画已配置通道拼出类型摘要，如 "位移 + 缩放"、"旋转(匀速) + 强度"
static std::string build_animation_type_string(Animation *anim) {
    std::vector<std::string> parts;
    // Position 通道按曲线类型区分：正弦振荡=位移，圆周轨道=轨道旋转
    if (AnimChannel *pos = anim->GetChannel(AnimProperty::Position)) {
        parts.push_back(pos->m_curve == AnimCurveType::Orbit ? "轨道旋转" : "位移");
    }
    if (anim->HasChannel(AnimProperty::Scale)) {
        parts.push_back("缩放");
    }
    // 旋转通道的曲线类型决定是匀速旋转还是正弦摆动
    if (AnimChannel *rot = anim->GetChannel(AnimProperty::Rotation)) {
        if (rot->m_curve == AnimCurveType::Spin) {
            parts.push_back("旋转(匀速)");
        } else {
            parts.push_back("旋转(摆动)");
        }
    }
    if (anim->HasChannel(AnimProperty::LightColor)) {
        parts.push_back("颜色");
    }
    if (anim->HasChannel(AnimProperty::LightIntensity)) {
        parts.push_back("强度");
    }
    if (parts.empty()) {
        return "未配置";
    }
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += " + ";
        result += parts[i];
    }
    return result;
}

void ToyEngineMainWindow::ShowAnimationProperties(IAnimTarget *target) {
    const auto &animations = m_renderer->GetAnimations();
    bool has_any = false;
    for (const auto &anim : animations) {
        if (anim->GetTarget() != target) {
            continue;
        }
        has_any = true;

        // 用动画对象指针作 ID，避免同名（或未命名）动画的 TreeNode 冲突
        ImGui::PushID(anim.get());
        std::string title = anim->GetName().empty() ? "未命名动画" : anim->GetName();
        const std::string label = title + "  [" + build_animation_type_string(anim.get()) + "]";
        // 默认展开：新建动画节点默认展开，类型摘要直接挂在标题上
        if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            // 总开关：关闭后 Update 直接返回，目标保持静态
            bool enabled = anim->IsEnabled();
            if (ImGui::Checkbox("启用##anim", &enabled)) {
                anim->SetEnabled(enabled);
            }

            // 位移通道：中心/振幅/频率 可编辑，写回后立即生效
            if (AnimChannel *tr = anim->GetChannel(AnimProperty::Position)) {
                ImGui::Separator();
                if (tr->m_curve == AnimCurveType::Orbit) {
                    // 圆周轨道：中心/半径(取 x)/频率（绕 Y 轴画圈，y 固定为中心高度）
                    ImGui::Text("轨道旋转");
                    ImGui::DragFloat3("中心##pos", glm::value_ptr(tr->center), 0.1f);
                    ImGui::DragFloat3("半径##pos", glm::value_ptr(tr->amplitude), 0.1f);
                    ImGui::DragFloat("频率##pos", &tr->m_frequency, 0.01f, 0.0f, 100.0f);
                } else {
                    ImGui::Text("位移");
                    // 控件标签带 ##pos 后缀：同动画多通道共享通用参数名，避免 ImGui ID 冲突
                    ImGui::DragFloat3("中心##pos", glm::value_ptr(tr->center), 0.1f);
                    ImGui::DragFloat3("振幅##pos", glm::value_ptr(tr->amplitude), 0.1f);
                    ImGui::DragFloat("频率##pos", &tr->m_frequency, 0.01f, 0.0f, 100.0f);
                }
            }

            // 缩放通道：基准/振幅/频率 可编辑（呼吸缩放）
            if (AnimChannel *sc = anim->GetChannel(AnimProperty::Scale)) {
                ImGui::Separator();
                ImGui::Text("缩放");
                ImGui::DragFloat3("基准##scl", glm::value_ptr(sc->center), 0.1f);
                ImGui::DragFloat3("振幅##scl", glm::value_ptr(sc->amplitude), 0.1f);
                ImGui::DragFloat("频率##scl", &sc->m_frequency, 0.01f, 0.0f, 100.0f);
            }

            // 旋转通道：匀速旋转与正弦摆动互斥，二选一；参数实时可编辑
            if (AnimChannel *rot = anim->GetChannel(AnimProperty::Rotation)) {
                ImGui::Separator();
                if (rot->m_curve == AnimCurveType::Spin) {
                    ImGui::Text("旋转 (匀速)");
                    ImGui::DragFloat3("起始角##rot", glm::value_ptr(rot->center), 1.0f);
                    ImGui::DragFloat3("角速度##rot", glm::value_ptr(rot->speed), 1.0f);
                } else {
                    ImGui::Text("旋转 (摆动)");
                    ImGui::DragFloat3("基准##rot", glm::value_ptr(rot->center), 1.0f);
                    ImGui::DragFloat3("振幅##rot", glm::value_ptr(rot->amplitude), 1.0f);
                    ImGui::DragFloat("频率##rot", &rot->m_frequency, 0.01f, 0.0f, 100.0f);
                }
            }

            // 灯光颜色通道：中心色/振幅/频率（颜色取 RGB 三通道）
            if (AnimChannel *clr = anim->GetChannel(AnimProperty::LightColor)) {
                ImGui::Separator();
                ImGui::Text("灯光颜色");
                ImGui::ColorEdit3("中心色##clr", glm::value_ptr(clr->center));
                ImGui::DragFloat3("振幅##clr", glm::value_ptr(clr->amplitude), 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("频率##clr", &clr->m_frequency, 0.01f, 0.0f, 100.0f);
            }

            // 灯光强度通道：中心/振幅/频率（标量，取 vec3.x）
            if (AnimChannel *inten = anim->GetChannel(AnimProperty::LightIntensity)) {
                ImGui::Separator();
                ImGui::Text("灯光强度");
                ImGui::DragFloat("中心##inten", &inten->center.x, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("振幅##inten", &inten->amplitude.x, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("频率##inten", &inten->m_frequency, 0.01f, 0.0f, 100.0f);
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    if (!has_any) {
        ImGui::TextDisabled("该对象未绑定动画");
    }
}

/* 
 * 材质窗口内容：渲染风格切换 + 材质参数编辑
 *
 * 材质由 Model 持有（Renderer::GetModelMaterial 取的是 model->GetMaterial()），
 * 共享的 Lit/Toon 风格技术不持有材质，因此这里改完无需任何同步动作：
 * 下一帧 Model::Draw 会把材质逐 mesh 上传，各模型互不干扰。
 * 纯色/纹理两套着色器不使用 gMaterial uniform，其下的修改只影响 Lit/Toon 的显示。
 */
void ToyEngineMainWindow::ShowModelMaterialProperties(Model *model) {
    // ---- 渲染风格（运行时切换，基础版：无参数调节）----
    // 四套标准着色器：纯色 / 纹理 / 光照 / 卡通，顺序与 RenderStyle 枚举保持一致
    ImGui::SeparatorText("渲染风格");
    static const char *kRenderStyleNames[] = {"纯色", "纹理", "光照", "卡通"};
    int styleIdx = static_cast<int>(m_renderer->GetModelStyle(model));
    if (ImGui::Combo("渲染风格", &styleIdx, kRenderStyleNames, IM_ARRAYSIZE(kRenderStyleNames))) {
        m_renderer->SetModelStyle(model, static_cast<RenderStyle>(styleIdx));
    }

    Material* material = m_renderer->GetModelMaterial(model);
    if (material != nullptr) {
        ImGui::SeparatorText("材质参数");
        if (!material->m_name.empty()) {
            ImGui::Text("材质名称: %s", material->m_name.c_str());
        }

        // Material 成员为 public，此处直接修改；材质由 Model::Draw 逐绘制自动上传，无需手动同步
        auto* mat = material;

        glm::vec3 ambient = mat->m_ambient_color;
        if (ImGui::ColorEdit3("环境光颜色", glm::value_ptr(ambient))) {
            mat->m_ambient_color = ambient;
        }

        glm::vec3 diffuse = mat->m_diffuse_color;
        if (ImGui::ColorEdit3("漫反射颜色", glm::value_ptr(diffuse))) {
            mat->m_diffuse_color = diffuse;
        }

        glm::vec3 specular = mat->m_specular_color;
        if (ImGui::ColorEdit3("镜面反射颜色", glm::value_ptr(specular))) {
            mat->m_specular_color = specular;
        }

        float shininess = mat->m_shininess;
        if (ImGui::DragFloat("光泽度", &shininess, 0.5f, 0.0f, 256.0f)) {
            mat->m_shininess = shininess;
        }
    }
}

// ---- 灯光属性编辑器 ----
// 可编辑：名称（只读）、位置、颜色、环境光、漫反射、镜面反射、衰减参数
// 根据灯光实际类型显示对应的可编辑属性
void ToyEngineMainWindow::ShowLightProperties() {
    Light* light = static_cast<Light*>(m_selected_object);

    ImGui::SeparatorText("基本信息");
    ImGui::Text("类型: %s", light->GetLightTypeName().c_str());
    ImGui::Text("名称: %s", light->GetName().c_str());

    // 开启/关闭开关：切换后实时影响该灯光是否参与光照计算
    bool enabled = light->IsEnabled();
    if (ImGui::Checkbox("启用##light", &enabled)) {
        light->SetEnabled(enabled);
    }

    // ---- 方向光 ----
    if (light->GetLightType() == LightTypeDirection) {
        DirectionLight* dirLight = static_cast<DirectionLight*>(light);

        ImGui::SeparatorText("方向");
        ImGui::DragFloat3("方向", glm::value_ptr(dirLight->m_direction), 0.1f);

        ImGui::SeparatorText("颜色");
        ImGui::ColorEdit3("颜色", glm::value_ptr(dirLight->m_color));

        ImGui::SeparatorText("环境光");
        ImGui::ColorEdit3("环境光颜色", glm::value_ptr(dirLight->m_ambient_color));
        ImGui::DragFloat("环境光强度", &dirLight->m_ambient_intensity, 0.01f, 0.0f, 10.0f);

        ImGui::SeparatorText("漫反射");
        ImGui::ColorEdit3("漫反射颜色", glm::value_ptr(dirLight->m_diffuse_color));
        ImGui::DragFloat("漫反射强度", &dirLight->m_diffuse_intensity, 0.01f, 0.0f, 10.0f);

        ImGui::SeparatorText("镜面反射");
        ImGui::ColorEdit3("镜面反射颜色", glm::value_ptr(dirLight->m_specular_color));
        ImGui::DragFloat("镜面反射强度", &dirLight->m_specular_intensity, 0.01f, 0.0f, 10.0f);
        return;
    }

    // ---- 聚光灯 ----
    if (light->GetLightType() == LightTypeSpot) {
        SpotLight* spotLight = static_cast<SpotLight*>(light);

        ImGui::SeparatorText("位置与方向");
        // 位置（DebugDraw 每帧直读 Position，gizmo 自动跟随，无需手动同步）
        ImGui::DragFloat3("位置", glm::value_ptr(spotLight->m_position), 0.1f);
        ImGui::DragFloat3("方向", glm::value_ptr(spotLight->m_direction), 0.1f);

        ImGui::SeparatorText("颜色");
        ImGui::ColorEdit3("颜色", glm::value_ptr(spotLight->m_color));

        ImGui::SeparatorText("环境光");
        ImGui::ColorEdit3("环境光颜色", glm::value_ptr(spotLight->m_ambient_color));
        ImGui::DragFloat("环境光强度", &spotLight->m_ambient_intensity, 0.01f, 0.0f, 10.0f);

        ImGui::SeparatorText("漫反射");
        ImGui::ColorEdit3("漫反射颜色", glm::value_ptr(spotLight->m_diffuse_color));
        ImGui::DragFloat("漫反射强度", &spotLight->m_diffuse_intensity, 0.01f, 0.0f, 10.0f);

        ImGui::SeparatorText("镜面反射");
        ImGui::ColorEdit3("镜面反射颜色", glm::value_ptr(spotLight->m_specular_color));
        ImGui::DragFloat("镜面反射强度", &spotLight->m_specular_intensity, 0.01f, 0.0f, 10.0f);

        ImGui::SeparatorText("衰减");
        ImGui::DragFloat("常数项", &spotLight->Attenuation.m_constant, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("线性项", &spotLight->Attenuation.m_linear, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("指数项", &spotLight->Attenuation.m_exp, 0.0001f, 0.0f, 0.1f);

        ImGui::SeparatorText("聚光锥角");
        ImGui::DragFloat("内锥角", &spotLight->m_cutoff, 0.5f, 0.0f, 90.0f);
        ImGui::DragFloat("外锥角", &spotLight->m_outer_cutoff, 0.5f, 0.0f, 90.0f);
        return;
    }

    // ---- 点光源（默认） ----
    PointLight* pointLight = static_cast<PointLight*>(light);

    ImGui::SeparatorText("位置");
    // 位置（DebugDraw 每帧直读 Position，gizmo 自动跟随，无需手动同步）
    ImGui::DragFloat3("位置", glm::value_ptr(pointLight->m_position), 0.1f);

    ImGui::SeparatorText("颜色");
    ImGui::ColorEdit3("颜色", glm::value_ptr(pointLight->m_color));

    ImGui::SeparatorText("环境光");
    ImGui::ColorEdit3("环境光颜色", glm::value_ptr(pointLight->m_ambient_color));
    ImGui::DragFloat("环境光强度", &pointLight->m_ambient_intensity, 0.01f, 0.0f, 10.0f);

    ImGui::SeparatorText("漫反射");
    ImGui::ColorEdit3("漫反射颜色", glm::value_ptr(pointLight->m_diffuse_color));
    ImGui::DragFloat("漫反射强度", &pointLight->m_diffuse_intensity, 0.01f, 0.0f, 10.0f);

    ImGui::SeparatorText("镜面反射");
    ImGui::ColorEdit3("镜面反射颜色", glm::value_ptr(pointLight->m_specular_color));
    ImGui::DragFloat("镜面反射强度", &pointLight->m_specular_intensity, 0.01f, 0.0f, 10.0f);

    ImGui::SeparatorText("衰减");
    ImGui::DragFloat("常数项", &pointLight->Attenuation.m_constant, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("线性项", &pointLight->Attenuation.m_linear, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("指数项", &pointLight->Attenuation.m_exp, 0.0001f, 0.0f, 0.1f);
}

// ---- 相机属性编辑器 ----
// 可编辑：名称（只读）、投影模式（透视/正交）、轨道参数（中心/半径/水平角/俯仰角）。
// 相机交互由 OrbitManipulator 承载，故轨道参数编辑的是操控器状态而非相机本体。
void ToyEngineMainWindow::ShowCameraProperties() {
    Camera* camera = static_cast<Camera*>(m_selected_object);
    OrbitManipulator* manipulator = m_renderer->GetManipulator();

    ImGui::SeparatorText("基本信息");
    ImGui::Text("类型: 摄像机");
    ImGui::Text("名称: %s", camera->GetName().c_str());

    ImGui::SeparatorText("投影");
    // 投影模式是摄像机属性：下拉框读写当前选中（即当前渲染用）摄像机的属性，
    // Renderer::SetProjectionType 写入后立即重算投影矩阵，切换即时生效
    const char* projTypes[] = { "透视", "正交" };
    int projType = static_cast<int>(camera->GetProjectionType());
    if (ImGui::Combo("投影模式", &projType, projTypes, IM_ARRAYSIZE(projTypes))) {
        m_renderer->SetProjectionType(static_cast<ProjectionType>(projType));
    }

    ImGui::SeparatorText("轨道参数");

    if (manipulator == nullptr) {
        ImGui::Text("（无操控器绑定）");
        return;
    }

    glm::vec3 center = manipulator->GetCenter();
    if (ImGui::DragFloat3("轨道中心", glm::value_ptr(center), 0.1f)) {
        manipulator->SetCenter(center);
    }

    float radius = manipulator->GetRadius();
    if (ImGui::DragFloat("半径", &radius, 0.1f, 0.5f, 100.0f)) {
        manipulator->SetRadius(radius);
    }

    float yaw = manipulator->GetYaw();
    if (ImGui::DragFloat("水平角", &yaw, 0.5f, -180.0f, 180.0f)) {
        manipulator->SetYaw(yaw);
    }

    float pitch = manipulator->GetPitch();
    if (ImGui::DragFloat("俯仰角", &pitch, 0.5f, -89.0f, 89.0f)) {
        manipulator->SetPitch(pitch);
    }
}

// ---- 地形属性编辑器 ----
// 显示只读统计信息和配置参数（平面尺寸、网格分辨率、高度缩放等）
void ToyEngineMainWindow::ShowTerrainProperties() {
    TerrainManager* terrain = static_cast<TerrainManager*>(m_selected_object);

    ImGui::SeparatorText("基本信息");
    ImGui::Text("类型: 地形");
    ImGui::Text("总三角形数: %d", terrain->GetTotalTriangleCount());

    // 配置参数（只读展示，运行时修改需要重新生成地形）
    ImGui::SeparatorText("配置");
    const TerrainConfig& cfg = terrain->GetConfig();
    ImGui::Text("平面尺寸: %.0f × %.0f", cfg.m_plane_size, cfg.m_plane_size);
    ImGui::Text("网格分辨率: %d × %d", cfg.m_resolution, cfg.m_resolution);
    ImGui::Text("高度缩放: %.1f", cfg.m_height_scale);
    ImGui::Text("噪声种子: %u", cfg.m_noise_seed);
}

// ---- 天空穹属性编辑器 ----
// 可编辑：地平线颜色、天顶颜色、地面雾色；只读：半径、分段数
void ToyEngineMainWindow::ShowSkyDomeProperties() {
    SkyDome* sky = static_cast<SkyDome*>(m_selected_object);

    ImGui::SeparatorText("只读信息");
    ImGui::Text("类型: 天空穹");
    ImGui::Text("半径: %.1f", sky->GetRadius());
    ImGui::Text("水平分段: %d", sky->GetSectors());
    ImGui::Text("垂直分段: %d", sky->GetStacks());

    // 可编辑颜色
    ImGui::SeparatorText("颜色");
    glm::vec3 horizonColor = sky->GetHorizonColor();
    if (ImGui::ColorEdit3("地平线颜色", glm::value_ptr(horizonColor))) {
        sky->SetHorizonColor(horizonColor);
    }

    glm::vec3 zenithColor = sky->GetZenithColor();
    if (ImGui::ColorEdit3("天顶颜色", glm::value_ptr(zenithColor))) {
        sky->SetZenithColor(zenithColor);
    }

    glm::vec3 groundColor = sky->GetGroundColor();
    if (ImGui::ColorEdit3("地面雾色", glm::value_ptr(groundColor))) {
        sky->SetGroundColor(groundColor);
    }
}

// ---- 粒子系统属性编辑器 ----
// 可编辑：发射器位置、发射速率、最大粒子数、生命周期、大小、速度、颜色、重力、阻力
void ToyEngineMainWindow::ShowParticleProperties() {
    if (m_selected_particle_index < 0
        || m_selected_particle_index >= static_cast<int>(m_renderer->GetParticleSystems().size())) {
        ImGui::Text("无效的粒子系统选择");
        return;
    }

    ParticleSystem* ps = m_renderer->GetParticleSystems()[m_selected_particle_index].get();
    ParticleEmitter* emitter = ps->GetEmitter();
    if (emitter == nullptr) {
        ImGui::Text("粒子发射器未初始化");
        return;
    }

    ImGui::SeparatorText("基本信息");
    ImGui::Text("类型: 粒子系统");
    ImGui::Text("索引: %d", m_selected_particle_index);

    // 位置
    ImGui::SeparatorText("位置");
    ImGui::DragFloat3("位置", glm::value_ptr(emitter->m_position), 0.1f);

    // 发射参数
    ImGui::SeparatorText("发射参数");
    ImGui::DragFloat("发射速率", &emitter->m_emit_rate, 1.0f, 0.0f, 10000.0f);
    ImGui::DragInt("最大粒子数", &emitter->m_max_particles, 10, 1, 50000);

    // 生命周期
    ImGui::SeparatorText("生命周期");
    ImGui::DragFloat("最小寿命", &emitter->m_min_life, 0.1f, 0.0f, 60.0f);
    ImGui::DragFloat("最大寿命", &emitter->m_max_life, 0.1f, 0.0f, 60.0f);

    // 大小
    ImGui::SeparatorText("大小");
    ImGui::DragFloat("初始最小", &emitter->m_min_size, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("初始最大", &emitter->m_max_size, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("结束最小", &emitter->m_min_size_end, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("结束最大", &emitter->m_max_size_end, 0.1f, 0.0f, 100.0f);

    // 速度
    ImGui::SeparatorText("速度");
    ImGui::DragFloat3("最小速度", glm::value_ptr(emitter->m_min_velocity), 0.1f);
    ImGui::DragFloat3("最大速度", glm::value_ptr(emitter->m_max_velocity), 0.1f);

    // 颜色：烟花配色表（只读，由 ParticleEmitter 内部调色板决定）
    ImGui::SeparatorText("颜色（烟花配色表）");
    {
        // 调色板与 particle_emitter.cpp 中 kFireworkPalette 保持一致
        static const float kPalette[][3] = {
            {1.00f, 0.84f, 0.00f}, // 金色
            {1.00f, 1.00f, 0.20f}, // 明黄
            {1.00f, 0.55f, 0.00f}, // 橙红
            {1.00f, 0.10f, 0.05f}, // 火红
            {1.00f, 0.25f, 0.50f}, // 粉红
            {1.00f, 0.00f, 1.00f}, // 品红
            {0.70f, 0.00f, 1.00f}, // 紫罗兰
            {0.00f, 0.40f, 1.00f}, // 蓝色
            {0.00f, 1.00f, 0.90f}, // 青色
            {0.10f, 1.00f, 0.20f}, // 翠绿
        };
        const float swatchSize = 18.0f;
        for (int i = 0; i < 10; i++) {
            ImGui::ColorButton(
                "##palswatch",
                ImVec4(kPalette[i][0], kPalette[i][1], kPalette[i][2], 1.0f),
                ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip,
                ImVec2(swatchSize, swatchSize)
            );
            if (i < 9) ImGui::SameLine();
        }
    }

    // 物理
    ImGui::SeparatorText("物理");
    ImGui::DragFloat3("重力", glm::value_ptr(emitter->m_gravity), 0.1f);
    ImGui::DragFloat("阻力", &emitter->m_drag, 0.01f, 0.0f, 1.0f);
}

// ---- FPS 曲线面板（默认收起，Ctrl+6 打开）----
// 首次打开时贴在 3D 视口底部的横条上，之后沿用 imgui.ini 保存的位置，
// 用户可自由拖动或停靠到任意节点。收起时不占视口空间。
void ToyEngineMainWindow::ShowFpsGraph() {
    // 视口矩形来自上一帧，首帧无效时交给 ImGui 默认放置
    if (m_viewport_width > 0.0f && m_viewport_height > 0.0f) {
        const float height = ImMax(m_viewport_height * 0.15f, 96.0f);
        ImGui::SetNextWindowPos(
            ImVec2(m_viewport_x, m_viewport_y + m_viewport_height - height),
            ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(
            ImVec2(m_viewport_width, height), ImGuiCond_FirstUseEver);
    }
    ImGui::Begin("FPS曲线", &m_show_fps_graph,
        ImGuiWindowFlags_NoCollapse);

    // 统计历史窗口内的平均/最低/最高帧率（vector 头尾为最早/最新采样）
    float sum = 0.0f;
    float minFps = std::numeric_limits<float>::max();
    float maxFps = 0.0f;
    for (float fps : m_fps_history) {
        sum += fps;
        minFps = std::min(minFps, fps);
        maxFps = std::max(maxFps, fps);
    }
    const size_t count = m_fps_history.size();
    const float avgFps = count > 0 ? sum / static_cast<float>(count) : 0.0f;
    const float curFps = count > 0 ? m_fps_history.back() : 0.0f;
    // 历史为空时用 0 占位，避免 minFps 初始值（float 最大值）暴露到界面
    const float minFpsDisp = count > 0 ? minFps : 0.0f;

    // 帧时间与当前帧率互为倒数，一并显示便于定位卡顿（ms 比 fps 更直观）
    const float frameMs = curFps > 0.0f ? 1000.0f / curFps : 0.0f;
    ImGui::Text("当前 %.1f (%.1f ms)   平均 %.1f   最低 %.1f   最高 %.1f",
                curFps, frameMs, avgFps, minFpsDisp, maxFps);

    if (count > 0) {
        // Y 轴范围动态跟随曲线数据：按窗口内最小/最大值各留 10% 余量，
        // 让曲线尽量填满绘图区高度，小幅波动也能明显看出
        float scaleMin = minFps;
        float scaleMax = maxFps;
        const float span = scaleMax - scaleMin;
        if (span >= 1.0f) {
            scaleMin -= span * 0.1f;
            scaleMax += span * 0.1f;
        } else {
            // 值域不足 1fps（曲线接近水平，如稳定 60）时人为撑开区间，避免除以零
            scaleMin -= 1.0f;
            scaleMax += 1.0f;
        }
        // overlay 文本直接叠在曲线右上角，收起统计行时也能一眼看到当前帧率
        static char overlay[32];
        std::snprintf(overlay, sizeof(overlay), "%.0f FPS", curFps);
        ImGui::PlotLines("##fps_curve", m_fps_history.data(), static_cast<int>(count),
                         0, overlay, scaleMin, scaleMax, ImVec2(-1.0f, 48.0f));
    } else {
        ImGui::Text("（暂无数据）");
    }

    ImGui::End();
}

/*
 * 阴影属性面板
 *
 * 全局阴影设置：开关、深度贴图预览入口。
 * 停靠于右节点（属性面板下方），通过菜单「视图 → 阴影属性」控制显示。
 */
void ToyEngineMainWindow::ShowShadowPropertiesPanel() {
    ImGui::Begin("阴影属性", &m_show_shadow_properties, ImGuiWindowFlags_NoCollapse);

    ImGui::SeparatorText("阴影");

    // 阴影开关：禁用时跳过深度 Pass，也不绑定阴影贴图
    bool shadowsEnabled = m_renderer->IsShadowsEnabled();
    if (ImGui::Checkbox("启用阴影", &shadowsEnabled)) {
        m_renderer->SetShadowsEnabled(shadowsEnabled);
    }

    // 阴影 bias 缩放：正交范围自适应后自阴影痤疮随光照角度/范围变化，现场微调（1.0 = 原始公式）
    float biasScale = m_renderer->GetShadowBiasScale();
    if (ImGui::SliderFloat("阴影偏置 (Bias)", &biasScale, 0.0f, 5.0f, "%.2f")) {
        m_renderer->SetShadowBiasScale(biasScale);
    }

    ImGui::SeparatorText("深度贴图");
    const unsigned int srcTex = m_renderer->GetShadowDepthTexture();
    const bool ready = m_renderer->IsShadowMapReady();
    if (srcTex == 0) {
        ImGui::TextWrapped("FBO 未创建（阴影未启用）。");
    } else if (!ready) {
        ImGui::TextWrapped("本帧未生成深度贴图（深度 Pass 未执行）。");
    } else {
        ImGui::TextWrapped("深度贴图已就绪。");
        // 点击打开深度贴图可视化面板
        if (ImGui::Button("查看深度贴图")) {
            m_show_shadow_depth_map = true;
        }
    }

    ImGui::SeparatorText("光源摄像机");
    // 方向光阴影在深度 Pass 中使用一个"光源视角的摄像机"，参数由 Renderer 每帧计算
    const ShadowCameraParams &shadowCam = m_renderer->GetShadowCameraParams();
    if (!shadowCam.m_available) {
        ImGui::TextWrapped("无已启用的方向光，本帧未生成光源摄像机。");
    } else {
        if (ImGui::TreeNode("位置 / 朝向")) {
            ImGui::Text("位置  (%.2f, %.2f, %.2f)", shadowCam.m_position.x, shadowCam.m_position.y, shadowCam.m_position.z);
            ImGui::Text("方向  (%.2f, %.2f, %.2f)", shadowCam.m_direction.x, shadowCam.m_direction.y, shadowCam.m_direction.z);
            ImGui::Text("注视  (%.2f, %.2f, %.2f)", shadowCam.m_look_at.x, shadowCam.m_look_at.y, shadowCam.m_look_at.z);
            ImGui::Text("Up    (%.2f, %.2f, %.2f)", shadowCam.m_up.x, shadowCam.m_up.y, shadowCam.m_up.z);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("正交投影")) {
            ImGui::Text("Left / Right : %.1f / %.1f", shadowCam.m_ortho_left, shadowCam.m_ortho_right);
            ImGui::Text("Bottom / Top : %.1f / %.1f", shadowCam.m_ortho_bottom, shadowCam.m_ortho_top);
            ImGui::Text("Near / Far   : %.1f / %.1f", shadowCam.m_near_plane, shadowCam.m_far_plane);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("矩阵")) {
            // 按行打印 4×4 矩阵（glm 列主序，m[col][row] 输出第 row 行）
            auto showMatrix = [](const char *label, const glm::mat4 &m) {
                if (ImGui::TreeNode(label)) {
                    for (int r = 0; r < 4; r++) {
                        ImGui::Text("%7.3f  %7.3f  %7.3f  %7.3f",
                                    m[0][r], m[1][r], m[2][r], m[3][r]);
                    }
                    ImGui::TreePop();
                }
            };
            showMatrix("lightView", shadowCam.m_light_view);
            showMatrix("lightProjection", shadowCam.m_light_projection);
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

/*
 * 后处理面板
 *
 * 色调映射与影调调整（Tone Mapping、曝光、饱和度、对比度），
 * 与阴影属性面板同栏 tab 切换，通过菜单「视图 → 后处理」控制显示。
 */
void ToyEngineMainWindow::ShowPostProcessPanel() {
    ImGui::Begin("后处理", &m_show_post_process, ImGuiWindowFlags_NoCollapse);

    ImGui::SeparatorText("色调映射");

    // 后处理 tone mapping 开关（对比 ACES+gamma 与直通输出）
    bool toneMapping = m_renderer->IsToneMappingEnabled();
    if (ImGui::Checkbox("Tone Mapping", &toneMapping)) {
        m_renderer->SetToneMappingEnabled(toneMapping);
    }

    // 曝光系数：tone mapping 前乘入 HDR 线性值，实时调整整体明暗
    float exposure = m_renderer->GetExposure();
    if (ImGui::SliderFloat("曝光 (Exposure)", &exposure, 0.1f, 4.0f, "%.2f")) {
        m_renderer->SetExposure(exposure);
    }

    ImGui::SeparatorText("影调");

    // 饱和度/对比度：tonemap+gamma 后统一调整，ACES 输出后按观感微调影调（默认 1.0 不调整）
    float saturation = m_renderer->GetSaturation();
    if (ImGui::SliderFloat("饱和度 (Saturation)", &saturation, 0.0f, 2.0f, "%.2f")) {
        m_renderer->SetSaturation(saturation);
    }
    float contrast = m_renderer->GetContrast();
    if (ImGui::SliderFloat("对比度 (Contrast)", &contrast, 0.5f, 2.0f, "%.2f")) {
        m_renderer->SetContrast(contrast);
    }

    if (ImGui::Button("重置为默认")) {
        m_renderer->SetToneMappingEnabled(true);
        m_renderer->SetExposure(1.0f);
        m_renderer->SetSaturation(1.0f);
        m_renderer->SetContrast(1.0f);
    }

    ImGui::End();
}

/*
 * 调试属性面板
 *
 * 集中放置渲染/编辑器调试相关的全局开关，与阴影属性面板相互独立：
 *   - 光源调试线框（DebugDraw）：控制场景中光源位置/范围 gizmo 的显示
 *   - 线框模式：地形/模型以线框渲染（GLPolygonMode 切换，阴影 Pass 不受影响）
 *   - 网格地面：XZ 平面世界网格辅助线，帮助判断空间方位
 *   - 视口背景色：运行时修改 glClearColor RGB（ColorEdit3 调色器）
 *   - 视野 FOV（度）：滑块实时调整透视投影范围
 * 后续新增调试项统一补充到这里。
 */
void ToyEngineMainWindow::ShowDebugPropertiesPanel() {
    ImGui::Begin("调试属性", &m_show_debug_properties, ImGuiWindowFlags_NoCollapse);

    // ---- 可视化开关组 ----
    if (ImGui::CollapsingHeader("可视化", ImGuiTreeNodeFlags_DefaultOpen)) {
        // 光源调试线框（DebugDraw）：控制场景中光源位置/范围 gizmo 的显示
        bool debugDraw = m_renderer->IsDebugDrawEnabled();
        if (ImGui::Checkbox("光源调试线框 (DebugDraw)", &debugDraw)) {
            m_renderer->SetDebugDrawEnabled(debugDraw);
        }

        // 线框模式：地形/模型以线框渲染（GLPolygonMode 切换，阴影 Pass 不受影响）
        bool wireframe = m_renderer->IsWireframeEnabled();
        if (ImGui::Checkbox("线框模式 (场景)", &wireframe)) {
            m_renderer->SetWireframeEnabled(wireframe);
        }

        // 网格地面辅助线：XZ 平面世界网格，帮助判断空间方位
        bool grid = m_renderer->IsGridEnabled();
        if (ImGui::Checkbox("网格地面", &grid)) {
            m_renderer->SetGridEnabled(grid);
        }

        // 法线可视化：启用时顶点法线以彩色线段显示（RGB 编码方向），用于检查法线朝向
        bool normalVis = m_renderer->IsNormalVisualizationEnabled();
        if (ImGui::Checkbox("法线可视化", &normalVis)) {
            m_renderer->SetNormalVisualizationEnabled(normalVis);
        }
        // 法线线段统一长度（世界空间单位），滑块实时调整，所有模型等长
        float normalLen = m_renderer->GetNormalLength();
        if (ImGui::SliderFloat("法线长度", &normalLen, 0.1f, 10.0f, "%.1f")) {
            m_renderer->SetNormalLength(normalLen);
        }

        // 光照范围可视化：在光源 gizmo 基础上叠加球形/锥形影响范围线框，直观展示衰减边界
        bool lightRange = m_renderer->IsLightRangeEnabled();
        if (ImGui::Checkbox("光照范围可视化", &lightRange)) {
            m_renderer->SetLightRangeEnabled(lightRange);
        }
    }

    // ---- 视口设置组 ----
    if (ImGui::CollapsingHeader("视口", ImGuiTreeNodeFlags_DefaultOpen)) {
        // 视口背景色：运行时修改 glClearColor RGB
        glm::vec3 clearColor = m_renderer->GetClearColor();
        if (ImGui::ColorEdit3("视口背景色", glm::value_ptr(clearColor))) {
            m_renderer->SetClearColor(clearColor);
        }

        // 视野 FOV（度）：滑块实时调整透视投影范围
        float fov = m_renderer->GetFov();
        if (ImGui::SliderFloat("FOV (度)", &fov, 10.0f, 160.0f, "%.1f")) {
            m_renderer->SetFov(fov);
        }
    }

    ImGui::End();
}

/*
 * 截图：把当前默认 framebuffer 内容保存为 PNG 文件
 *
 * 调用时机：RenderFrame 末尾、glfwSwapBuffers 之前——此时场景、后处理、
 * ImGui 界面均已绘制到默认 framebuffer，读回的正是用户看到的完整一帧。
 *
 * 实现要点：
 *   - 按 framebuffer 实际像素尺寸读取（Retina 屏幕下像素尺寸通常为窗口尺寸的 2 倍）；
 *   - 垂直翻转：OpenGL 行序自底向上（左下角为原点），PNG 期望自顶向下（左上角为原点），
 *     故读回后逐行反转；
 *   - 行对齐设为 1 字节（GL_PACK_ALIGNMENT = 1），避免行尾填充字节造成错位；
 *   - 文件名带时间戳，避免重复截图相互覆盖。
 */
void ToyEngineMainWindow::SaveScreenshot() {
    // 获取默认 framebuffer 的实际像素尺寸（Retina 下与窗口逻辑尺寸不同）
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    if (fbWidth <= 0 || fbHeight <= 0) {
        return;
    }

    // RGB 三通道像素缓冲（1 字节对齐，杜绝行尾 padding 干扰）
    std::vector<unsigned char> pixels(static_cast<size_t>(fbWidth) * fbHeight * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, fbWidth, fbHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // 垂直翻转行序（OpenGL 原点在左下，图片文件原点在左上）
    std::vector<unsigned char> flipped(static_cast<size_t>(fbWidth) * fbHeight * 3);
    const size_t rowBytes = static_cast<size_t>(fbWidth) * 3;
    for (int y = 0; y < fbHeight; ++y) {
        std::memcpy(&flipped[static_cast<size_t>(fbHeight - 1 - y) * rowBytes],
                    &pixels[static_cast<size_t>(y) * rowBytes], rowBytes);
    }

    // 生成带时间戳的文件名：screenshot_YYYYMMDD_HHMMSS.png
    std::time_t now = std::time(nullptr);
    std::tm tm = *std::localtime(&now);
    char filename[128];
    std::snprintf(filename, sizeof(filename), "screenshot_%04d%02d%02d_%02d%02d%02d.png",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);

    // 写入 PNG（3 通道、原始行字节数 = 宽 × 3）
    if (stbi_write_png(filename, fbWidth, fbHeight, 3, flipped.data(),
                       static_cast<int>(rowBytes)) != 0) {
        printf("[Screenshot] saved: %s (%dx%d)\n", filename, fbWidth, fbHeight);
    } else {
        printf("[Screenshot] FAILED to write: %s\n", filename);
    }
}

/*
 * 阴影深度贴图可视化调试面板
 *
 * 把阴影深度贴图（GL_DEPTH_COMPONENT 只写格式）作为纹理显示出来，
 * 用于诊断阴影问题（如"z>0 全黑"）。
 *
 * 为什么不能直接 ImGui::Image 原始深度纹理：
 *   深度纹理是 GL_DEPTH_COMPONENT 格式、不含颜色附件，ImGui 的 OpenGL3
 *   后端用颜色采样器（sampler2D）采样它时，不同驱动对 R 通道的返回值
 *   行为不一致，可能整片黑/灰难以辨认。因此这里把深度读回 CPU、
 *   归一化为灰度（近→黑，远→白）并上传到一张 RGBA8 纹理再展示，显示确定可靠。
 *
 * 性能：面板/渲染期间仅在首次显示和源深度贴图变化时读回一次，并复用
 * 缓存纹理避免每帧重建。
 */
void ToyEngineMainWindow::ShowShadowDepthMapPanel() {
    const unsigned int srcTex = m_renderer->GetShadowDepthTexture();
    const bool ready = m_renderer->IsShadowMapReady();

    ImGui::Begin("阴影深度贴图", &m_show_shadow_depth_map, ImGuiWindowFlags_NoCollapse);

    if (srcTex == 0) {
        ImGui::TextWrapped("阴影 FBO 未创建（阴影可能未启用）。");
    } else if (!ready) {
        ImGui::TextWrapped("本帧未生成阴影深度贴图（深度 Pass 未执行）。");
    } else {
        // 显示宽度：预览纹理与源深度贴图同尺寸，这里仅决定 ImGui 显示多大
        const int displayW = static_cast<int>(ImGui::GetContentRegionAvail().x);
        // 仅当源纹理变化时才重新读回/重建缓存，避免每帧 CPU 拷贝
        if (m_shadow_depth_preview_tex == 0 || srcTex != m_last_displayed_depth_tex) {
            RebuildShadowDepthPreview(srcTex);
        }

        if (m_shadow_depth_preview_tex != 0) {
            // 用预览纹理的宽高比自适应缩放显示，保持内容不被拉伸
            const float aspect = static_cast<float>(m_shadow_depth_preview_w) /
                                static_cast<float>(m_shadow_depth_preview_h);
            const float maxW = (float)displayW;
            ImVec2 displaySize(maxW, maxW / aspect);
            ImGui::Image((ImTextureID)(intptr_t)m_shadow_depth_preview_tex, displaySize);
            ImGui::TextWrapped("近处=黑，远处=白。空白区域 = 无深度（未写入）或边界外被照亮。");
        }
    }

    ImGui::End();
}

/*
 * 重建阴影深度预览纹理
 *
 * 从源深度纹理读回深度数据，归一化为灰度（近→黑，远→白）后上传到
 * 一张缓存复用（m_shadow_depth_preview_tex）的 RGBA8 纹理。深度纹理内部
 * 格式为 GL_DEPTH_COMPONENT/GL_FLOAT，故用 GL_FLOAT 读回原始 [0,1] 深度。
 */
void ToyEngineMainWindow::RebuildShadowDepthPreview(unsigned int srcTex) {
    // 查询源深度纹理的实际尺寸
    int texW = 0, texH = 0;
    glBindTexture(GL_TEXTURE_2D, srcTex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);
    if (texW <= 0 || texH <= 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    // 读回原始 float 深度（匹配内部格式 GL_R32F / GL_RED 红色通道）
    std::vector<float> depth(static_cast<size_t>(texW) * texH);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, depth.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // 归一化：遍历找到实际最小/最大深度（排除边界外 1.0 白底与未写入区），
    // 将 [min,max] 线性映射到 [0,1] 灰度，避免整体偏黑看不清分布
    float minD = 1.0f, maxD = 0.0f;
    for (float d : depth) {
        if (d < 1.0f) {          // 排除边界外的 1.0（被照亮区），集中归一化有效深度
            if (d < minD) minD = d;
            if (d > maxD) maxD = d;
        }
    }
    const float range = (maxD > minD) ? (maxD - minD) : 1.0f;

    // 生成灰度像素（R=G=B，A=255）
    std::vector<unsigned char> pixels(static_cast<size_t>(texW) * texH * 4);
    for (size_t i = 0; i < depth.size(); ++i) {
        float g = (depth[i] >= 1.0f) ? 1.0f : (depth[i] - minD) / range;
        unsigned char c = static_cast<unsigned char>(g * 255.0f);
        pixels[i * 4 + 0] = c;
        pixels[i * 4 + 1] = c;
        pixels[i * 4 + 2] = c;
        pixels[i * 4 + 3] = 255;
    }

    // 复用缓存纹理：预览纹理尺寸应等于源深度贴图尺寸（texW×texH）。
    // 之前曾误用「面板可用宽度」作为预览纹理尺寸，导致向更小的纹理上传
    // 完整的 2048×2048 子图触发 GL_INVALID_VALUE、上传失败，预览恒为全黑。
    // 现在预览纹理与源同尺寸，显示时再由 ImGui::Image 按面板宽度缩放。
    if (m_shadow_depth_preview_tex == 0 ||
        m_shadow_depth_preview_w != texW || m_shadow_depth_preview_h != texH) {
        if (m_shadow_depth_preview_tex != 0) {
            glDeleteTextures(1, &m_shadow_depth_preview_tex);
        }
        glGenTextures(1, &m_shadow_depth_preview_tex);
        glBindTexture(GL_TEXTURE_2D, m_shadow_depth_preview_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_shadow_depth_preview_w = texW;
    m_shadow_depth_preview_h = texH;
    m_last_displayed_depth_tex = srcTex;
}

// ---- 视口角落的屏幕空间坐标轴 gizmo ----
// 参照 erhe（ImViewGuizmo）在视口角落叠加世界坐标系指示器：
// 用相机视图矩阵的旋转部分将三色六轴（X 红 / Y 绿 / Z 蓝）投影到屏幕，
// 各轴按视图深度排序并淡化远处轴，末端带圆点把手与 X/Y/Z 标签。
// 位置固定在视口右上角，避开属性面板与边缘。
void ToyEngineMainWindow::DrawViewportAxisGizmo() {
    // 首帧 DockSpace 布局未完成时视口尺寸无效，跳过绘制
    if (m_viewport_width <= 0.0f || m_viewport_height <= 0.0f) {
        return;
    }

    Axis* axis = m_renderer->GetAxis();
    Camera* camera = m_renderer->GetCamera();
    if (axis == nullptr || camera == nullptr) {
        return;
    }

    // gizmo 直径 = 基准直径(256) × 缩放(0.6) ≈ 154px，半径约 77px
    // 中心放置在视口右上角，向左/向下各留出足够边距避开面板与边缘
    const float gizmoRadius = axis->GetScale() * 256.0f * 0.5f;
    const float margin = gizmoRadius * 0.5f;
    const ImVec2 center{
        m_viewport_x + m_viewport_width - gizmoRadius - margin,
        m_viewport_y + gizmoRadius + margin
    };

    axis->Draw(camera->GetViewMatrix(), center);
}

// ---- 3D 拾取辅助函数 ----

// 拾取半径（世界单位）：用于点光源/聚光灯/粒子发射器的射线-球求交。
// 取较小的固定值，点击 gizmo 附近即可命中，无需精确点中对象中心。
static constexpr float kPickRadius = 0.5f;

/*
 * 射线-三角形求交（Möller–Trumbore 算法）
 *
 * 返回是否相交，相交时通过 outT 输出射线参数 t（命中点 = origin + t * dir）。
 * 只计算正面相交（det 过小视为与三角面片平行，不视为命中），
 * t 必须大于 0（射线前进方向上的命中）。
 */
static bool RayTriangleIntersect(
    const glm::vec3& origin, const glm::vec3& dir,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
    float& outT) {
    const float kEpsilon = 1e-6f;

    // 计算两个边向量，构造以 v0 为原点的局部坐标系
    const glm::vec3 edge1 = v1 - v0;
    const glm::vec3 edge2 = v2 - v0;

    // Möller–Trumbore：先用 dir × edge2 得到 P 向量，det 为三角形法线与射线的点积
    const glm::vec3 p = glm::cross(dir, edge2);
    const float det = glm::dot(edge1, p);

    // |det| 过小说明射线与三角形所在平面几乎平行（掠射），跳过避免除零与病态结果
    if (std::fabs(det) < kEpsilon) {
        return false;
    }
    const float invDet = 1.0f / det;

    // 射线起点相对三角形顶点 v0 的偏移向量
    const glm::vec3 tVec = origin - v0;

    // 重心坐标 u（沿 edge1 方向）
    const float u = glm::dot(tVec, p) * invDet;
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    // 重心坐标 v（沿 edge2 方向）
    const glm::vec3 q = glm::cross(tVec, edge1);
    const float v = glm::dot(dir, q) * invDet;
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    // 距离参数 t（沿射线方向），必须为正值（只取射线前方的命中）
    const float t = glm::dot(edge2, q) * invDet;
    if (t <= 0.0f) {
        return false;
    }

    outT = t;
    return true;
}

/*
 * 射线-球体求交
 *
 * 用于点光源/聚光灯/粒子发射器等"有位置、无网格"对象的拾取。
 * 解二次方程 |origin + t*dir - center|² = r²，返回最近的正根 t。
 */
static bool RaySphereIntersect(
    const glm::vec3& origin, const glm::vec3& dir,
    const glm::vec3& center, float radius,
    float& outT) {
    // 将射线方程代入球方程后展开为关于 t 的二次方程 a*t² + b*t + c = 0，
    // 其中 dir 为单位向量所以 a == 1，可直接用简化判别式
    const glm::vec3 oc = origin - center;
    const float b = glm::dot(oc, dir);
    const float c = glm::dot(oc, oc) - radius * radius;

    // 判别式 < 0：射线与球不相交
    const float discriminant = b * b - c;
    if (discriminant < 0.0f) {
        return false;
    }

    // 取最近的正根 t = -b - sqrt(disc)；若为负则取另一个根（起点在球内的情况）
    const float sqrtDisc = std::sqrt(discriminant);
    float t = -b - sqrtDisc;
    if (t < 0.0f) {
        t = -b + sqrtDisc;
    }
    if (t < 0.0f) {
        return false;
    }

    outT = t;
    return true;
}

// ---- 选择管理 ----
void ToyEngineMainWindow::SelectObject(void* obj, const std::string& type) {
    m_selected_object = obj;
    m_selected_object_type = type;

    // 选中模型时置位：让「模型」tab 成为默认可视窗口（其它类型保持原活动 tab）
    if (type == "Model") {
        m_focus_model_window = true;
    }
}

void ToyEngineMainWindow::ClearSelection() {
    m_selected_object = nullptr;
    m_selected_object_type.clear();
    m_selected_particle_index = -1;
}

/*
 * 3D 拾取：从鼠标点击位置发出一条拾取射线，与场景中所有可拾取对象求交，
 * 选中"最近"的命中对象并联动属性面板（SelectObject）与渲染器高亮（SetPickHighlight）。
 *
 * 拾取流程：
 *   1. 鼠标屏幕坐标（ImGui 逻辑坐标）→ 归一化设备坐标 NDC（Y 翻转，origin 在左上）；
 *   2. 用「逆(投影 × 视图)」矩阵把近/远裁剪面上的同一像素反投影回世界空间，
 *      构成世界射线（近点 = 射线原点，远点 - 近点 = 射线方向）；
 *   3. 依次与模型（逐三角形 Möller–Trumbore，顶点经 GetWorldMatrix 变换）、
 *      点/聚光灯（射线-球）、粒子发射器（射线-球）、地形（单位矩阵世界坐标三角形）求交，
 *      保留 t 最小（沿射线最近）的命中；
 *   4. 命中 → SelectObject + SetPickHighlight(命中对象包围盒)；未命中 → 清除选择与高亮。
 */
void ToyEngineMainWindow::PerformPick() {
    // 渲染器或视口尚未就绪（首帧 DockSpace 布局未完成）时无法拾取
    if (!m_renderer || m_viewport_width <= 0.0f || m_viewport_height <= 0.0f) {
        return;
    }

    // 取当前鼠标位置（ImGui 窗口逻辑坐标，位于中央 3D 视口内时才拾取；
    // 视口外的点击已被 ImGui WantCaptureMouse 拦截，此处仅做兜底）
    double cursorX = 0.0, cursorY = 0.0;
    glfwGetCursorPos(m_window, &cursorX, &cursorY);

    // 屏幕像素坐标 → NDC：先归一化到 [0,1]，再映射到 [-1,1]（NDC 的 Y 轴向上）
    const float ndcX = static_cast<float>((cursorX - m_viewport_x) / m_viewport_width) * 2.0f - 1.0f;
    const float ndcY = 1.0f - static_cast<float>((cursorY - m_viewport_y) / m_viewport_height) * 2.0f;

    // 拾取必须使用与渲染完全一致的投影/视图矩阵（当前相机 + 当前宽高比）
    const glm::mat4 invViewProj = glm::inverse(
        m_renderer->GetProjectionMatrix() * m_renderer->GetViewMatrix());

    // 近/远裁剪面上的同一点（NDC z = -1 / +1）反投影到世界空间，构成拾取射线
    const glm::vec4 nearWorld4 = invViewProj * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    const glm::vec4 farWorld4  = invViewProj * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
    const glm::vec3 rayOrigin = glm::vec3(nearWorld4) / nearWorld4.w;
    const glm::vec3 rayDir = glm::normalize(glm::vec3(farWorld4) / farWorld4.w - rayOrigin);

    // 记录沿射线最近（t 最小）的命中对象
    float bestT = std::numeric_limits<float>::max();
    void* hitObject = nullptr;
    std::string hitType;
    int hitParticleIndex = -1;

    // ---- 模型：逐三角形求交 ----
    // 把射线变换到模型本地空间求交（避免逐顶点做世界变换）：
    // 世界坐标射线 × 逆世界矩阵 → 本地坐标射线，等价于把网格顶点留在本地坐标系做测试。
    for (const auto& model : m_renderer->GetModels()) {
        if (model == nullptr) {
            continue;
        }
        const glm::mat4 invWorld = glm::inverse(model->GetWorldMatrix());
        const glm::vec3 localOrigin = glm::vec3(invWorld * glm::vec4(rayOrigin, 1.0f));
        const glm::vec3 localDir = glm::normalize(glm::vec3(invWorld * glm::vec4(rayDir, 0.0f)));

        for (const auto& mesh : model->GetMeshes()) {
            // 只有三角形图元才能按「每 3 个索引一个三角形」拾取；
            // GL_LINES/GL_POINTS 等调试类网格无面片概念，跳过
            if (mesh == nullptr || mesh->m_draw_mode != GL_TRIANGLES || mesh->m_indices.size() < 3) {
                continue;
            }
            const std::vector<Vertex>& verts = mesh->m_vertices;
            const auto& idx = mesh->m_indices;
            // 采用索引三角形的图元拓扑（GL_TRIANGLES），每 3 个索引构成一个三角形
            for (size_t i = 0; i + 2 < idx.size(); i += 3) {
                float t = 0.0f;
                if (RayTriangleIntersect(localOrigin, localDir,
                        verts[idx[i]].m_position,
                        verts[idx[i + 1]].m_position,
                        verts[idx[i + 2]].m_position, t) && t < bestT) {
                    bestT = t;
                    hitObject = model.get();
                    hitType = "Model";
                }
            }
        }
    }

    // ---- 点光源 / 聚光灯：射线-球求交（方向光无位置，跳过）----
    // 拾取半径取 0.5（世界单位），点击光源 gizmo 附近即可命中，无需精确点中
    for (const auto& light : m_renderer->GetLights()) {
        if (light == nullptr || !light->IsEnabled()) {
            continue;
        }
        glm::vec3 lightPos;
        switch (light->GetLightType()) {
            case LightTypePoint:
                lightPos = static_cast<PointLight*>(light.get())->m_position;
                break;
            case LightTypeSpot:
                lightPos = static_cast<SpotLight*>(light.get())->m_position;
                break;
            default:
                continue; // 方向光没有世界位置可拾取
        }

        float t = 0.0f;
        if (RaySphereIntersect(rayOrigin, rayDir, lightPos, kPickRadius, t) && t < bestT) {
            bestT = t;
            hitObject = light.get();
            hitType = "Light";
        }
    }

    // ---- 粒子系统：以发射器位置做射线-球求交 ----
    const auto& particleSystems = m_renderer->GetParticleSystems();
    for (size_t i = 0; i < particleSystems.size(); ++i) {
        const ParticleSystem* ps = particleSystems[i].get();
        if (ps == nullptr || ps->GetEmitter() == nullptr) {
            continue;
        }

        float t = 0.0f;
        if (RaySphereIntersect(rayOrigin, rayDir, ps->GetEmitter()->m_position, kPickRadius, t) && t < bestT) {
            bestT = t;
            hitObject = const_cast<ParticleSystem*>(ps);
            hitType = "Particle";
            hitParticleIndex = static_cast<int>(i);
        }
    }

    // ---- 地形：单位矩阵绘制，网格顶点即为世界坐标，直接逐三角形求交 ----
    TerrainManager* terrain = m_renderer->GetTerrainManager();
    if (terrain != nullptr) {
        Mesh* terrainMesh = terrain->GetTerrainMesh();
        if (terrainMesh != nullptr && terrainMesh->m_indices.size() >= 3) {
            const std::vector<Vertex>& verts = terrainMesh->m_vertices;
            const std::vector<GLuint>& idx = terrainMesh->m_indices;
            for (size_t i = 0; i + 2 < idx.size(); i += 3) {
                float t = 0.0f;
                if (RayTriangleIntersect(rayOrigin, rayDir,
                        verts[idx[i]].m_position,
                        verts[idx[i + 1]].m_position,
                        verts[idx[i + 2]].m_position, t) && t < bestT) {
                    bestT = t;
                    hitObject = terrain;
                    hitType = "Terrain";
                }
            }
        }
    }

    // ---- 命中分发：选中 + 高亮；未命中则清除选择与高亮 ----
    if (hitObject == nullptr) {
        ClearSelection();
        m_renderer->ClearPickHighlight();
        return;
    }

    SelectObject(hitObject, hitType);
    if (hitType == "Particle") {
        m_selected_particle_index = hitParticleIndex;
    }

    // 计算命中对象的包围盒用于线框高亮：
    //   模型/地形 —— 遍历网格顶点（含变换）求世界空间 AABB；
    //   灯光/粒子 —— 直接以位置 ± 半径构成小盒
    glm::vec3 aabbMin(0.0f), aabbMax(0.0f);
    if (hitType == "Model" || hitType == "Terrain") {
        // 初始化 AABB 为反向极大/极小值
        aabbMin = glm::vec3(std::numeric_limits<float>::max());
        aabbMax = glm::vec3(-std::numeric_limits<float>::max());

        // 收集命中对象的网格指针
        std::vector<const Mesh*> meshes;
        glm::mat4 world(1.0f);
        if (hitType == "Model") {
            Model* model = static_cast<Model*>(hitObject);
            for (const auto& mesh : model->GetMeshes()) {
                meshes.push_back(mesh.get());
            }
            world = model->GetWorldMatrix();
        } else {
            Mesh* terrainMesh = terrain->GetTerrainMesh();
            if (terrainMesh != nullptr) {
                meshes.push_back(terrainMesh);
            }
        }

        for (const Mesh* mesh : meshes) {
            if (mesh == nullptr) {
                continue;
            }
            for (const Vertex& v : mesh->m_vertices) {
                const glm::vec3 worldPos = glm::vec3(world * glm::vec4(v.m_position, 1.0f));
                aabbMin = glm::min(aabbMin, worldPos);
                aabbMax = glm::max(aabbMax, worldPos);
            }
        }

        // 顶点列表为空时回退为无高亮（正常模型/地形必有顶点，此处仅防御）
        if (aabbMax.x < aabbMin.x) {
            m_renderer->ClearPickHighlight();
            return;
        }
    } else {
        // 灯光 / 粒子：以命中位置为中心的小包围盒
        glm::vec3 center(0.0f);
        if (hitType == "Light") {
            auto* light = static_cast<Light*>(hitObject);
            if (light->GetLightType() == LightTypePoint) {
                center = static_cast<PointLight*>(light)->m_position;
            } else {
                center = static_cast<SpotLight*>(light)->m_position;
            }
        } else if (hitType == "Particle") {
            center = static_cast<ParticleSystem*>(hitObject)->GetEmitter()->m_position;
        }
        aabbMin = center - glm::vec3(kPickRadius);
        aabbMax = center + glm::vec3(kPickRadius);
    }

    m_renderer->SetPickHighlight(aabbMin, aabbMax);
}

// ---- 鼠标事件 ----
// 所有 ImGui 捕获判断已在 GLFW 回调（MouseButtonCallback / CursorPosCallback / ScrollCallback）中完成

// 区分"点击"与"拖拽"的位移阈值（像素）：
// 左键按下到松开位移小于该值视为点击（触发 3D 拾取），
// 大于等于该值视为拖拽（用于轨道相机旋转/平移，不触发拾取）。
static constexpr double kClickDragThresholdPx = 5.0;

void ToyEngineMainWindow::OnMouseLeftButtonDown() {
    // 记录左键按下位置，供松开时判断是否为"点击"（位移 < 阈值）
    glfwGetCursorPos(m_window, &m_mouse_down_x, &m_mouse_down_y);
}

void ToyEngineMainWindow::OnMouseLeftButtonUp() {
    // 松开时计算与按下位置的位移，小于阈值判定为"点击"并触发 3D 拾取；
    // 拖拽（位移较大）仅用于相机控制，不触发拾取。
    double cx = 0.0, cy = 0.0;
    glfwGetCursorPos(m_window, &cx, &cy);

    const double dx = cx - m_mouse_down_x;
    const double dy = cy - m_mouse_down_y;
    if (dx * dx + dy * dy < kClickDragThresholdPx * kClickDragThresholdPx) {
        PerformPick();
    }
}

void ToyEngineMainWindow::OnMouseRightButtonDown() {
    m_camera_panning = true;
}

void ToyEngineMainWindow::OnMouseRightButtonUp() {
    m_camera_panning = false;
}

void ToyEngineMainWindow::OnMouseMove(double deltaX, double deltaY) {
    if (!m_renderer) return;

    // 相机绕轨道中心旋转（鼠标左键拖动）：交互交给操控器，相机只接收结果
    if (m_mouse_left_pressed) {
        auto *manipulator = m_renderer->GetManipulator();
        if (manipulator) {
            float sensitivity = 0.5f;
            manipulator->Orbit(
                static_cast<float>(-deltaX * sensitivity),
                static_cast<float>(-deltaY * sensitivity));
        }
    }

    // 相机平移（鼠标右键拖动）：轨道中心沿视图右/上方向移动
    if (m_camera_panning) {
        auto *manipulator = m_renderer->GetManipulator();
        if (manipulator) {
            float panSpeed = 0.01f;
            manipulator->Pan(
                static_cast<float>(deltaX * panSpeed),
                static_cast<float>(-deltaY * panSpeed));
        }
    }
}

void ToyEngineMainWindow::OnMouseWheel(double delta) {
    if (m_renderer) {
        auto *manipulator = m_renderer->GetManipulator();
        if (manipulator) {
            float zoomSpeed = 0.1f;
            manipulator->Zoom(static_cast<float>(delta * zoomSpeed));
        }
    }
}

void ToyEngineMainWindow::Cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // 渲染器须在窗口销毁与 glfwTerminate 之前释放 OpenGL 资源
    m_renderer.reset();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}
