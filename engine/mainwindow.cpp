#include <glad/gl.h>
#include "mainwindow.h"
#include "globals.h"
#include "config.h"
#include "renderer.h"
#include "model/model.h"
#include "light/light.h"
#include "camera/camera.h"
#include "terrain/terrain_manager.h"
#include "sky/sky_dome.h"
#include "particle/particle_system.h"
#include "particle/particle_emitter.h"

// OpenGL函数由renderer.cpp提供

// Dear ImGui
#include <imgui.h>
// imgui_internal.h：DockBuilder*（DockBuilderAddNode/SplitNode/DockWindow/Finish/GetCentralNode）
// 及 ImGuiDockNode 结构体（中央节点 Pos/Size）定义所在，仅在 mainwindow.cpp 使用
#include <imgui_internal.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>
#include <glm/gtc/type_ptr.hpp>

// GLFW错误回调
void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

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
            self->m_mouseLeftPressed = true;
        } else {
            self->OnMouseLeftButtonUp();
            self->m_mouseLeftPressed = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            self->OnMouseRightButtonDown();
            self->m_mouseRightPressed = true;
        } else {
            self->OnMouseRightButtonUp();
            self->m_mouseRightPressed = false;
        }
    }
}

void ToyEngineMainWindow::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // 先让 ImGui 处理（更新 WantCaptureMouse 状态）
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

    auto* self = static_cast<ToyEngineMainWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    // 计算并更新鼠标增量基线
    double deltaX = xpos - self->m_currentMouseX;
    double deltaY = ypos - self->m_currentMouseY;
    self->m_currentMouseX = xpos;
    self->m_currentMouseY = ypos;

    // ImGui 正在捕获鼠标时，不驱动相机，并重置相机交互状态避免残留
    if (ImGui::GetIO().WantCaptureMouse) {
        self->m_cameraPanning = false;
        return;
    }

    if (self->m_mouseLeftPressed || self->m_mouseRightPressed) {
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
}

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
        m_windowWidth = gConfig->Window.WindowWidth;
        m_windowHeight = gConfig->Window.WindowHeight;
    } catch (...) {
        m_windowWidth = 1280;
        m_windowHeight = 720;
        std::cerr << "Warning: Failed to load config file, using default window size" << std::endl;
    }

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Toy Engine", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(m_window, this);

    // 初始化鼠标位置基线，供 CursorPosCallback 计算增量
    glfwGetCursorPos(m_window, &m_currentMouseX, &m_currentMouseY);

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
            "./resource/font/微软雅黑.ttf", 18.0f, nullptr,
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
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 注册 GLFW 输入回调（ImGui 优先模式），替代原来的轮询方式
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetCursorPosCallback(m_window, CursorPosCallback);
    glfwSetScrollCallback(m_window, ScrollCallback);
    glfwSetKeyCallback(m_window, KeyCallback);

    // 初始化渲染器
    m_renderer = new Renderer();
    m_renderer->init(m_windowWidth, m_windowHeight);

    m_lastTime = static_cast<float>(glfwGetTime());
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
    m_deltaTime = currentTime - m_lastTime;
    m_lastTime = currentTime;

    m_renderer->update(static_cast<long long>(m_deltaTime * 1000));

    // 获取实际 framebuffer 像素尺寸（Retina 下通常为窗口 points 尺寸的 2 倍）。
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    // ---- ImGui 帧先行：DockSpace 完成布局后才能查询中央节点矩形 ----
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 创建/渲染 DockSpace，并将中央节点矩形写入 m_viewportX/Y/Width/Height
    CreateDockSpace();

    // 将中央节点矩形从 ImGui 逻辑坐标（左上原点、Y 向下）换算为
    // framebuffer 像素坐标（左下原点、Y 向上）：像素 = 逻辑值 × 缩放比
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    const float xScale = (displaySize.x > 0.0f) ? static_cast<float>(fbWidth) / displaySize.x : 1.0f;
    const float yScale = (displaySize.y > 0.0f) ? static_cast<float>(fbHeight) / displaySize.y : 1.0f;

    const int viewportX = static_cast<int>(m_viewportX * xScale);
    const int viewportY = static_cast<int>(static_cast<float>(fbHeight) - (m_viewportY + m_viewportHeight) * yScale);
    const int viewportW = static_cast<int>(m_viewportWidth * xScale);
    const int viewportH = static_cast<int>(m_viewportHeight * yScale);

    // 中央节点尺寸异常（首帧布局尚未完成）时回退到全窗口视口
    if (viewportW <= 0 || viewportH <= 0) {
        m_renderer->resize(fbWidth, fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
    } else {
        m_renderer->resize(viewportW, viewportH);
        glViewport(viewportX, viewportY, viewportW, viewportH);
    }
    m_renderer->draw(static_cast<long long>(m_deltaTime * 1000));

    // 恢复完整 framebuffer 视口给 ImGui 使用
    glViewport(0, 0, fbWidth, fbHeight);

    // 绘制面板（资源列表/属性面板停靠于 DockSpace 左右节点，状态条浮动于视口底部）
    CreateUI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}

/*
 * 创建/渲染 DockSpace 并建立初始三栏布局
 *
 * 布局：DockBuilder 将 DockSpace 竖直分为左/中/右三栏——
 *   - 左节点：资源列表（占 25% 宽度）
 *   - 右节点：属性面板（占 30% 宽度）
 *   - 中央节点：留空，作为 3D 渲染视口（PassthruCentralNode 模式，输入穿透用于相机控制）
 *
 * 仅首帧执行 DockBuilder 初始化；此后 DockSpace 完全接管布局，用户可自由
 * 拖拽/浮动/调整各面板。每帧结束时将中央节点矩形记录到 m_viewport* 成员，
 * 供 RenderFrame 换算 glViewport 以及状态条定位使用。
 */
void ToyEngineMainWindow::CreateDockSpace() {
    ImGuiID dockspaceId = ImGui::GetID("ToyEngineDockSpace");

    // 首次运行时用 DockBuilder 建立初始布局（之后保留用户调整结果）
    if (!m_dockspaceInitialized) {
        m_dockspaceInitialized = true;

        // 清空可能的残留节点，重新创建 DockSpace 根节点
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

        // 依次切分：先分出左栏（25%），再在剩余空间中分出右栏（30%），
        // 剩余部分即中央 3D 视口节点
        ImGuiID mainNode = dockspaceId;
        ImGuiID leftNode = ImGui::DockBuilderSplitNode(
            mainNode, ImGuiDir_Left, 0.25f, nullptr, &mainNode);
        ImGuiID rightNode = ImGui::DockBuilderSplitNode(
            mainNode, ImGuiDir_Right, 0.30f, nullptr, &mainNode);

        // 将面板窗口按标题绑定到对应节点（窗口标题必须与停靠目标一致）
        ImGui::DockBuilderDockWindow("资源列表", leftNode);
        ImGui::DockBuilderDockWindow("属性", rightNode);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    // 全屏宿主窗口承载菜单栏 + DockSpace：无标题、无边框，仅作为两者的容器
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
        ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar(3);

    // 主菜单栏（退出/资源/属性）：必须位于 DockSpace 之前渲染，
    // DockSpace 的可用区域会自动扣除菜单栏高度
    CreateMenuBar();

    // 渲染 DockSpace：PassthruCentralNode = 中央节点保持为空且输入穿透，
    // 让 GLFW 回调（相机控制）在中央区域可正常收到鼠标事件
    ImGui::DockSpace(dockspaceId, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    // 记录中央节点矩形（ImGui 逻辑坐标，左上原点、Y 向下），供 RenderFrame 换算
    ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspaceId);
    if (centralNode) {
        m_viewportX = centralNode->Pos.x;
        m_viewportY = centralNode->Pos.y;
        m_viewportWidth = centralNode->Size.x;
        m_viewportHeight = centralNode->Size.y;
    }
}

/*
 * 主菜单栏（退出 / 面板）
 *
 * 菜单结构：
 *   - 退出：请求关闭窗口（与 ESC 键行为一致，ESC 在 KeyCallback 中处理）
 *   - 面板：子菜单，包含
 *       - 资源：切换左侧资源列表面板的显示 / 隐藏（勾选状态 = 面板可见）
 *       - 属性：切换右侧属性面板的显示 / 隐藏（勾选状态 = 面板可见）
 *
 * 必须在宿主窗口 Begin() 之后、DockSpace() 之前调用，
 * 这样 DockSpace 可用区域自动扣除菜单栏高度，中央节点随之正确下移。
 */
void ToyEngineMainWindow::CreateMenuBar() {
    if (ImGui::BeginMenuBar()) {
        // 退出：点击后设置关闭标志，主循环在下一帧退出
        if (ImGui::MenuItem("退出", "Esc")) {
            glfwSetWindowShouldClose(m_window, true);
        }

        // 面板：子菜单收纳所有可开关面板；MenuItem 的第四个参数为选中状态
        // 指针（bool*），点击时自动切换并显示勾选标记，与面板可见性绑定
        if (ImGui::BeginMenu("面板")) {
            ImGui::MenuItem("资源", nullptr, &m_showResourceList);
            ImGui::MenuItem("属性", nullptr, &m_showProperties);
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
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
    if (m_showResourceList) {
        CreateResourceListPanel();
    }

    // 属性面板：停靠于右节点
    if (m_showProperties) {
        CreatePropertiesPanel();
    }

    // 视口底部浮动状态条（FPS/投影方式）
    if (m_showViewportStatusBar) {
        ShowViewportStatusBar();
    }
}

/*
 * 左栏：资源列表面板
 *
 * 显示 world.yaml 中定义的所有资源，按类型分组：
 *   - 摄像机（可切换当前摄像机）
 *   - 灯光（可选中编辑属性）
 *   - 模型（可选中编辑属性）
 *   - 地形（单例，可选中查看配置）
 *   - 天空穹（单例，可选中编辑颜色）
 *   - 粒子系统（可选中编辑发射器参数）
 */
void ToyEngineMainWindow::CreateResourceListPanel() {
    // 停靠于 DockSpace 左节点：不能带 NoMove/NoResize，否则无法被 DockBuilder 停靠
    // 与用户拖拽调整
    ImGui::Begin("资源列表", &m_showResourceList,
        ImGuiWindowFlags_NoCollapse);

    // ---- 摄像机 ----
    if (ImGui::TreeNodeEx("摄像机", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto cameras = m_renderer->GetCameras();
        for (size_t i = 0; i < cameras.size(); ++i) {
            const auto& camera = cameras[i];
            std::string displayName = camera->GetName().empty()
                ? "Camera " + std::to_string(i) : camera->GetName();
            bool isSelected = (m_selectedObject == camera
                && m_selectedObjectType == "Camera"
                && m_currentCameraIndex == static_cast<int>(i));

            if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                m_renderer->SwitchCamera(static_cast<int>(i));
                m_currentCameraIndex = static_cast<int>(i);
                SelectObject(m_renderer->GetCamera(), "Camera");
            }
        }
        ImGui::TreePop();
    }

    // ---- 灯光 ----
    if (ImGui::TreeNodeEx("灯光", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto lights = m_renderer->GetLights();
        for (size_t i = 0; i < lights.size(); ++i) {
            auto light = lights[i];
            if (light == nullptr) continue;
            std::string nodeName = light->GetName() + "##light" + std::to_string(i);
            bool isSelected = (m_selectedObject == light && m_selectedObjectType == "Light");
            if (ImGui::Selectable(nodeName.c_str(), isSelected)) {
                SelectObject(light, "Light");
            }
        }
        ImGui::TreePop();
    }

    // ---- 模型 ----
    if (ImGui::TreeNodeEx("模型", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto models = m_renderer->GetModels();
        for (size_t i = 0; i < models.size(); ++i) {
            auto model = models[i];
            if (model == nullptr) continue;
            std::string nodeName = model->GetName() + "##model" + std::to_string(i);
            bool isSelected = (m_selectedObject == model && m_selectedObjectType == "Model");
            if (ImGui::Selectable(nodeName.c_str(), isSelected)) {
                SelectObject(model, "Model");
            }
        }
        ImGui::TreePop();
    }

    // ---- 地形（单例） ----
    if (auto terrain = m_renderer->GetTerrainManager()) {
        bool isSelected = (m_selectedObjectType == "Terrain");
        if (ImGui::Selectable("地形", isSelected)) {
            SelectObject(terrain, "Terrain");
        }
    }

    // ---- 天空穹（单例） ----
    if (auto sky = m_renderer->GetSkyDome()) {
        bool isSelected = (m_selectedObjectType == "SkyDome");
        if (ImGui::Selectable("天空穹", isSelected)) {
            SelectObject(sky, "SkyDome");
        }
    }

    // ---- 粒子系统 ----
    auto& particles = m_renderer->GetParticleSystems();
    if (!particles.empty()) {
        if (ImGui::TreeNodeEx("粒子系统", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (size_t i = 0; i < particles.size(); ++i) {
                auto emitter = particles[i]->GetEmitter();
                if (emitter == nullptr) continue;
                // 使用发射器位置作为显示名
                std::string displayName = "Particle " + std::to_string(i);
                bool isSelected = (m_selectedObjectType == "Particle"
                    && m_selectedParticleIndex == static_cast<int>(i));
                if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                    SelectObject(particles[i], "Particle");
                    m_selectedParticleIndex = static_cast<int>(i);
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
    ImGui::Begin("属性", &m_showProperties,
        ImGuiWindowFlags_NoCollapse);

    if (m_selectedObject == nullptr) {
        ImGui::Text("请选择一个资源来编辑属性");
        ImGui::End();
        return;
    }

    if (m_selectedObjectType == "Model") {
        ShowModelProperties();
    } else if (m_selectedObjectType == "Light") {
        ShowLightProperties();
    } else if (m_selectedObjectType == "Camera") {
        ShowCameraProperties();
    } else if (m_selectedObjectType == "Terrain") {
        ShowTerrainProperties();
    } else if (m_selectedObjectType == "SkyDome") {
        ShowSkyDomeProperties();
    } else if (m_selectedObjectType == "Particle") {
        ShowParticleProperties();
    }

    ImGui::End();
}

// ---- 模型属性编辑器 ----
// 可编辑：名称（只读）、位置、缩放、旋转
void ToyEngineMainWindow::ShowModelProperties() {
    Model* model = static_cast<Model*>(m_selectedObject);

    ImGui::Text("类型: 模型");
    ImGui::Separator();
    ImGui::Text("名称: %s", model->GetName().c_str());
    ImGui::Separator();

    glm::vec3 position = model->GetPosition();
    if (ImGui::DragFloat3("位置", glm::value_ptr(position), 0.1f)) {
        model->SetTranslate(position);
    }

    glm::vec3 scale = model->GetScale();
    if (ImGui::DragFloat3("缩放", glm::value_ptr(scale), 0.1f)) {
        model->SetScale(scale);
    }

    float rotation = model->GetRotation();
    if (ImGui::DragFloat("旋转 (度)", &rotation, 1.0f)) {
        model->SetRotate(rotation);
    }
}

// ---- 灯光属性编辑器 ----
// 可编辑：名称（只读）、位置、颜色、环境光、漫反射、镜面反射、衰减参数
void ToyEngineMainWindow::ShowLightProperties() {
    PointLight* light = static_cast<PointLight*>(m_selectedObject);

    ImGui::Text("类型: 点光源");
    ImGui::Separator();
    ImGui::Text("名称: %s", light->GetName().c_str());
    ImGui::Separator();

    // 位置
    if (ImGui::DragFloat3("位置", glm::value_ptr(light->Position), 0.1f)) {
        // 同步更新光源模型位置
        if (light->GetModel()) {
            light->GetModel()->SetPosition(light->Position);
        }
    }

    // 颜色
    ImGui::ColorEdit3("颜色", glm::value_ptr(light->Color));

    ImGui::Separator();
    ImGui::Text("环境光");
    ImGui::ColorEdit3("环境光颜色", glm::value_ptr(light->AmbientColor));
    ImGui::DragFloat("环境光强度", &light->AmbientIntensity, 0.01f, 0.0f, 10.0f);

    ImGui::Separator();
    ImGui::Text("漫反射");
    ImGui::ColorEdit3("漫反射颜色", glm::value_ptr(light->DiffuseColor));
    ImGui::DragFloat("漫反射强度", &light->DiffuseIntensity, 0.01f, 0.0f, 10.0f);

    ImGui::Separator();
    ImGui::Text("镜面反射");
    ImGui::ColorEdit3("镜面反射颜色", glm::value_ptr(light->SpecularColor));
    ImGui::DragFloat("镜面反射强度", &light->SpecularIntensity, 0.01f, 0.0f, 10.0f);

    ImGui::Separator();
    ImGui::Text("衰减");
    ImGui::DragFloat("常数项", &light->Attenuation.Constant, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("线性项", &light->Attenuation.Linear, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("指数项", &light->Attenuation.Exp, 0.0001f, 0.0f, 0.1f);
}

// ---- 相机属性编辑器 ----
// 可编辑：名称（只读）、位置、目标点、上方向
void ToyEngineMainWindow::ShowCameraProperties() {
    Camera* camera = static_cast<Camera*>(m_selectedObject);

    ImGui::Text("类型: 摄像机");
    ImGui::Separator();
    ImGui::Text("名称: %s", camera->GetName().c_str());
    ImGui::Separator();

    glm::vec3 position = camera->GetPosition();
    if (ImGui::DragFloat3("位置", glm::value_ptr(position), 0.1f)) {
        camera->SetPosition(position);
    }

    glm::vec3 target = camera->m_target;
    if (ImGui::DragFloat3("目标点", glm::value_ptr(target), 0.1f)) {
        camera->m_target = target;
    }

    glm::vec3 up = camera->m_world_up;
    if (ImGui::DragFloat3("上方向", glm::value_ptr(up), 0.01f)) {
        camera->m_world_up = up;
    }

    ImGui::Separator();
    ImGui::DragFloat("移动速度", &camera->m_move_speed, 0.1f, 0.1f, 100.0f);
    ImGui::DragFloat("鼠标灵敏度", &camera->m_mouse_sensitivity, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("缩放", &camera->m_zoom, 0.1f, 1.0f, 90.0f);
}

// ---- 地形属性编辑器 ----
// 显示只读统计信息和配置参数（chunk大小、分辨率、渲染距离等）
void ToyEngineMainWindow::ShowTerrainProperties() {
    TerrainManager* terrain = static_cast<TerrainManager*>(m_selectedObject);

    ImGui::Text("类型: 地形");
    ImGui::Separator();

    // 只读统计信息
    ImGui::Text("活跃 Chunk 数: %d", terrain->GetActiveChunkCount());
    ImGui::Text("总三角形数: %d", terrain->GetTotalTriangleCount());
    ImGui::Separator();

    // 配置参数（只读展示，运行时修改需要重新生成地形）
    const TerrainConfig& cfg = terrain->GetConfig();
    float chunkSize = cfg.chunkSize;
    ImGui::DragFloat("Chunk 大小", &chunkSize, 1.0f, 10.0f, 500.0f);

    int baseRes = cfg.baseResolution;
    ImGui::DragInt("基础分辨率", &baseRes, 1, 8, 256);

    int renderDist = cfg.renderDistance;
    ImGui::DragInt("渲染距离", &renderDist, 1, 1, 50);

    int unloadDist = cfg.unloadDistance;
    ImGui::DragInt("卸载距离", &unloadDist, 1, 1, 100);

    float heightScale = cfg.heightScale;
    ImGui::DragFloat("高度缩放", &heightScale, 0.1f, 0.0f, 100.0f);

    ImGui::Text("噪声种子: %u", cfg.noiseSeed);
}

// ---- 天空穹属性编辑器 ----
// 可编辑：地平线颜色、天顶颜色；只读：半径、分段数
void ToyEngineMainWindow::ShowSkyDomeProperties() {
    SkyDome* sky = static_cast<SkyDome*>(m_selectedObject);

    ImGui::Text("类型: 天空穹");
    ImGui::Separator();

    // 只读参数
    ImGui::Text("半径: %.1f", sky->GetRadius());
    ImGui::Text("水平分段: %d", sky->GetSectors());
    ImGui::Text("垂直分段: %d", sky->GetStacks());
    ImGui::Separator();

    // 可编辑颜色
    glm::vec3 horizonColor = sky->GetHorizonColor();
    if (ImGui::ColorEdit3("地平线颜色", glm::value_ptr(horizonColor))) {
        sky->SetHorizonColor(horizonColor);
    }

    glm::vec3 zenithColor = sky->GetZenithColor();
    if (ImGui::ColorEdit3("天顶颜色", glm::value_ptr(zenithColor))) {
        sky->SetZenithColor(zenithColor);
    }
}

// ---- 粒子系统属性编辑器 ----
// 可编辑：发射器位置、发射速率、最大粒子数、生命周期、大小、速度、颜色、重力、阻力
void ToyEngineMainWindow::ShowParticleProperties() {
    if (m_selectedParticleIndex < 0
        || m_selectedParticleIndex >= static_cast<int>(m_renderer->GetParticleSystems().size())) {
        ImGui::Text("无效的粒子系统选择");
        return;
    }

    ParticleSystem* ps = m_renderer->GetParticleSystems()[m_selectedParticleIndex];
    ParticleEmitter* emitter = ps->GetEmitter();
    if (emitter == nullptr) {
        ImGui::Text("粒子发射器未初始化");
        return;
    }

    ImGui::Text("类型: 粒子系统");
    ImGui::Text("索引: %d", m_selectedParticleIndex);
    ImGui::Separator();

    // 位置
    ImGui::DragFloat3("位置", glm::value_ptr(emitter->Position), 0.1f);

    // 发射参数
    ImGui::Separator();
    ImGui::Text("发射参数");
    ImGui::DragFloat("发射速率", &emitter->EmitRate, 1.0f, 0.0f, 10000.0f);
    ImGui::DragInt("最大粒子数", &emitter->MaxParticles, 10, 1, 50000);

    // 生命周期
    ImGui::Separator();
    ImGui::Text("生命周期");
    ImGui::DragFloat("最小寿命", &emitter->MinLife, 0.1f, 0.0f, 60.0f);
    ImGui::DragFloat("最大寿命", &emitter->MaxLife, 0.1f, 0.0f, 60.0f);

    // 大小
    ImGui::Separator();
    ImGui::Text("大小");
    ImGui::DragFloat("初始最小", &emitter->MinSize, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("初始最大", &emitter->MaxSize, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("结束最小", &emitter->MinSizeEnd, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("结束最大", &emitter->MaxSizeEnd, 0.1f, 0.0f, 100.0f);

    // 速度
    ImGui::Separator();
    ImGui::Text("速度");
    ImGui::DragFloat3("最小速度", glm::value_ptr(emitter->MinVelocity), 0.1f);
    ImGui::DragFloat3("最大速度", glm::value_ptr(emitter->MaxVelocity), 0.1f);

    // 颜色
    ImGui::Separator();
    ImGui::Text("初始颜色");
    ImGui::ColorEdit3("最小颜色", glm::value_ptr(emitter->MinColor));
    ImGui::ColorEdit3("最大颜色", glm::value_ptr(emitter->MaxColor));

    ImGui::Text("结束颜色");
    ImGui::ColorEdit3("最小结束颜色", glm::value_ptr(emitter->MinColorEnd));
    ImGui::ColorEdit3("最大结束颜色", glm::value_ptr(emitter->MaxColorEnd));

    // 物理
    ImGui::Separator();
    ImGui::Text("物理");
    ImGui::DragFloat3("重力", glm::value_ptr(emitter->Gravity), 0.1f);
    ImGui::DragFloat("阻力", &emitter->Drag, 0.01f, 0.0f, 1.0f);
}

// ---- 中格视口底部状态条 ----
// 以悬浮条覆盖在中间 3D 视口底部，不占用独立网格行，保持 3x1 网格布局
void ToyEngineMainWindow::ShowViewportStatusBar() {
    const float barHeight = ImGui::GetFrameHeight() + 8.0f;
    // 状态条悬浮于中央节点（3D 视口）底部，位置随 DockSpace 布局动态跟随
    ImGui::SetNextWindowPos(ImVec2(m_viewportX, m_viewportY + m_viewportHeight - barHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(m_viewportWidth, barHeight), ImGuiCond_Always);

    ImGui::Begin("##ViewportStatusBar", &m_showViewportStatusBar,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

    ImGui::Text("FPS: %.1f  |  %dx%d", m_renderer->GetFPS(), m_windowWidth, m_windowHeight);
    ImGui::SameLine();

    const char* projTypes[] = { "透视", "正交" };
    int projType = static_cast<int>(m_renderer->GetProjectionType());
    ImGui::SameLine();
    if (ImGui::Combo("##proj", &projType, projTypes, IM_ARRAYSIZE(projTypes))) {
        m_renderer->SerProjectionType(static_cast<ProjectionType>(projType));
    }

    ImGui::End();
}

// ---- 选择管理 ----
void ToyEngineMainWindow::SelectObject(void* obj, const std::string& type) {
    m_selectedObject = obj;
    m_selectedObjectType = type;
}

void ToyEngineMainWindow::ClearSelection() {
    m_selectedObject = nullptr;
    m_selectedObjectType.clear();
    m_selectedParticleIndex = -1;
}

// ---- 鼠标事件 ----
// 所有 ImGui 捕获判断已在 GLFW 回调（MouseButtonCallback / CursorPosCallback / ScrollCallback）中完成
void ToyEngineMainWindow::OnMouseLeftButtonDown() {
}

void ToyEngineMainWindow::OnMouseLeftButtonUp() {
}

void ToyEngineMainWindow::OnMouseRightButtonDown() {
    m_cameraPanning = true;
}

void ToyEngineMainWindow::OnMouseRightButtonUp() {
    m_cameraPanning = false;
}

void ToyEngineMainWindow::OnMouseMove(double deltaX, double deltaY) {
    if (!m_renderer) return;

    // 相机绕世界原点旋转（鼠标左键拖动）
    if (m_mouseLeftPressed) {
        auto camera = m_renderer->GetCamera();
        if (camera) {
            float sensitivity = 0.5f;
            camera->OrbitAroundOrigin(
                static_cast<float>(-deltaX * sensitivity),
                static_cast<float>(-deltaY * sensitivity));
        }
    }

    // 相机平移（鼠标右键拖动）
    if (m_cameraPanning) {
        auto camera = m_renderer->GetCamera();
        if (camera) {
            float panSpeed = 0.01f;
            camera->Pan(
                static_cast<float>(deltaX * panSpeed),
                static_cast<float>(-deltaY * panSpeed));
        }
    }
}

void ToyEngineMainWindow::OnMouseWheel(double delta) {
    if (m_renderer) {
        auto camera = m_renderer->GetCamera();
        if (camera) {
            float zoomSpeed = 0.1f;
            camera->Zoom(static_cast<float>(delta * zoomSpeed));
        }
    }
}

void ToyEngineMainWindow::Cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_renderer) {
        delete m_renderer;
        m_renderer = nullptr;
    }

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}
