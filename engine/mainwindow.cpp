#include "mainwindow.h"
#include "globals.h"
#include "config.h"
#include "renderer.h"
#include "model/model.h"
#include "light/light.h"
#include "camera/camera.h"

// OpenGL函数由renderer.cpp提供

// Dear ImGui
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>

// GLFW错误回调
void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

ToyEngineMainWindow::~ToyEngineMainWindow() {
    Cleanup();
}

bool ToyEngineMainWindow::Initialize() {
    // 初始化GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // 配置GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 从配置文件读取窗口尺寸
    try {
        gConfig = Config::LoadFromYaml("./resource/world.yaml");
        m_windowWidth = gConfig->Window.WindowWidth;
        m_windowHeight = gConfig->Window.WindowHeight;
    } catch (...) {
        // 如果加载失败，使用默认尺寸
        m_windowWidth = 1280;
        m_windowHeight = 720;
        std::cerr << "Warning: Failed to load config file, using default window size" << std::endl;
    }

    // 创建窗口
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Toy Engine", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // 启用垂直同步
    
    // 注册鼠标滚轮回调
    glfwSetScrollCallback(m_window, ScrollCallback);

    // OpenGL上下文由renderer初始化

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // 注释掉停靠功能，因为可能不支持
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 加载中文字体
    std::cout << "正在加载中文字体..." << std::endl;
    
    // 检查字体文件是否存在
    FILE* fontFile = fopen("./resource/font/微软雅黑.ttf", "rb");
    if (fontFile) {
        fclose(fontFile);
        std::cout << "找到字体文件" << std::endl;
        
        ImFont* chineseFont = io.Fonts->AddFontFromFileTTF("./resource/font/微软雅黑.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        if (chineseFont) {
            std::cout << "✅ 成功加载中文字体" << std::endl;
            // 设置为默认字体
            io.FontDefault = chineseFont;
        } else {
            std::cerr << "❌ 加载中文字体失败" << std::endl;
        }
    } else {
        std::cerr << "❌ 找不到字体文件: ./resource/font/微软雅黑.ttf" << std::endl;
    }

    ImGui::StyleColorsDark();

    // 初始化ImGui后端
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 初始化渲染器
    m_renderer = new Renderer();
    m_renderer->init(m_windowWidth, m_windowHeight);
    try {
        m_renderer->LoadWorldFromFile("./resource/world.yaml");
    } catch (...) {
        std::cerr << "Warning: Failed to load world from file" << std::endl;
    }

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
    // ESC键退出
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }
    
    // 获取鼠标位置
    glfwGetCursorPos(m_window, &m_currentMouseX, &m_currentMouseY);
    
    // 处理鼠标按键
    int leftMouseButton = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT);
    int rightMouseButton = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT);
    
    bool leftPressed = (leftMouseButton == GLFW_PRESS);
    bool rightPressed = (rightMouseButton == GLFW_PRESS);
    
    // 鼠标左键按下/释放检测
    if (leftPressed && !m_mouseLeftPressed) {
        // 左键刚按下
        OnMouseLeftButtonDown();
    } else if (!leftPressed && m_mouseLeftPressed) {
        // 左键刚释放
        OnMouseLeftButtonUp();
    }
    
    // 鼠标右键按下/释放检测
    if (rightPressed && !m_mouseRightPressed) {
        // 右键刚按下
        OnMouseRightButtonDown();
    } else if (!rightPressed && m_mouseRightPressed) {
        // 右键刚释放
        OnMouseRightButtonUp();
    }
    
    // 更新鼠标按键状态
    m_mouseLeftPressed = leftPressed;
    m_mouseRightPressed = rightPressed;
    
    // 处理鼠标移动
    if (m_mouseLeftPressed || m_mouseRightPressed) {
        double deltaX = m_currentMouseX - m_lastMouseX;
        double deltaY = m_currentMouseY - m_lastMouseY;
        
        if (deltaX != 0.0 || deltaY != 0.0) {
            OnMouseMove(deltaX, deltaY);
        }
    }
    
    // 处理鼠标滚轮
    // 注意：GLFW使用回调处理滚轮事件，这里暂时注释掉
    // double scrollX, scrollY;
    // glfwGetScrollOffset(m_window, &scrollX, &scrollY);
    // if (scrollY != 0.0) {
    //     OnMouseWheel(scrollY);
    // }
    
    // 更新鼠标位置
    m_lastMouseX = m_currentMouseX;
    m_lastMouseY = m_currentMouseY;
}

void ToyEngineMainWindow::RenderFrame() {
    // 计算时间差
    float currentTime = static_cast<float>(glfwGetTime());
    m_deltaTime = currentTime - m_lastTime;
    m_lastTime = currentTime;

    // 更新渲染器
    m_renderer->update(static_cast<long long>(m_deltaTime * 1000));

    // 开始ImGui帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 创建UI
    CreateUI();

    // 渲染3D场景
    m_renderer->draw(static_cast<long long>(m_deltaTime * 1000));

    // 渲染ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}

void ToyEngineMainWindow::CreateUI() {
    // 注释掉停靠空间，因为可能不支持
    // ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    // 创建主菜单栏
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("文件")) {
            if (ImGui::MenuItem("打开场景")) {}
            if (ImGui::MenuItem("保存场景")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("退出", "Alt+F4")) {
                glfwSetWindowShouldClose(m_window, true);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("视图")) {
            ImGui::MenuItem("场景树", nullptr, &m_showSceneTree);
            ImGui::MenuItem("属性面板", nullptr, &m_showProperties);
            ImGui::MenuItem("工具栏", nullptr, &m_showToolbar);
            ImGui::MenuItem("状态栏", nullptr, &m_showStatusBar);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("帮助")) {
            if (ImGui::MenuItem("关于")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // 创建各个面板
    if (m_showToolbar) {
        CreateToolbar();
    }
    
    if (m_showSceneTree) {
        CreateSceneTreePanel();
    }
    
    if (m_showProperties) {
        CreatePropertiesPanel();
    }
    
    if (m_showStatusBar) {
        CreateStatusBar();
    }
}

void ToyEngineMainWindow::CreateToolbar() {
    ImGui::Begin("工具栏", &m_showToolbar, ImGuiWindowFlags_NoCollapse);
    
    if (ImGui::Button("播放")) {}
    ImGui::SameLine();
    if (ImGui::Button("暂停")) {}
    ImGui::SameLine();
    if (ImGui::Button("停止")) {}
    ImGui::SameLine();
    
    ImGui::Separator();
    ImGui::SameLine();
    
    if (ImGui::Button("添加模型")) {}
    ImGui::SameLine();
    if (ImGui::Button("添加光源")) {}
    
    ImGui::End();
}

void ToyEngineMainWindow::CreateSceneTreePanel() {
    ImGui::Begin("场景树", &m_showSceneTree, ImGuiWindowFlags_NoCollapse);
    
    // 显示模型列表
    if (ImGui::TreeNode("模型")) {
        auto models = m_renderer->GetModels();
        for (size_t i = 0; i < models.size(); ++i) {
            auto model = models[i];
            std::string nodeName = model->GetName() + "##" + std::to_string(i);
            if (ImGui::Selectable(nodeName.c_str(), 
                m_selectedObject == model && m_selectedObjectType == "Model")) {
                SelectObject(model);
                m_selectedObjectType = "Model";
            }
        }
        ImGui::TreePop();
    }
    
    // 显示光源列表
    if (ImGui::TreeNode("光源")) {
        auto lights = m_renderer->GetLights();
        for (size_t i = 0; i < lights.size(); ++i) {
            auto light = lights[i];
            std::string nodeName = light->GetName() + "##" + std::to_string(i);
            if (ImGui::Selectable(nodeName.c_str(),
                m_selectedObject == light && m_selectedObjectType == "Light")) {
                SelectObject(light);
                m_selectedObjectType = "Light";
            }
        }
        ImGui::TreePop();
    }
    
    // 显示相机
    if (ImGui::TreeNode("相机")) {
        auto camera = m_renderer->GetCamera();
        if (camera && ImGui::Selectable("主相机", 
            m_selectedObject == camera && m_selectedObjectType == "Camera")) {
            SelectObject(camera);
            m_selectedObjectType = "Camera";
        }
        ImGui::TreePop();
    }
    
    ImGui::End();
}

void ToyEngineMainWindow::CreatePropertiesPanel() {
    ImGui::Begin("属性", &m_showProperties, ImGuiWindowFlags_NoCollapse);
    
    if (m_selectedObject) {
        UpdateSelectedObjectProperties();
    } else {
        ImGui::Text("请选择一个对象来编辑属性");
    }
    
    ImGui::End();
}

void ToyEngineMainWindow::CreateStatusBar() {
    ImGui::Begin("状态栏", &m_showStatusBar, 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    
    // 显示FPS
    ImGui::Text("FPS: %.1f", m_renderer->GetFPS());
    ImGui::SameLine();
    // ImGui::SeparatorVertical(); // 可能不支持
    ImGui::SameLine();
    
    // 显示窗口尺寸
    ImGui::Text("尺寸: %dx%d", m_windowWidth, m_windowHeight);
    ImGui::SameLine();
    // ImGui::SeparatorVertical(); // 可能不支持
    ImGui::SameLine();
    
    // 显示投影模式
    const char* projTypes[] = { "透视", "正交" };
    int projType = static_cast<int>(m_renderer->GetProjectionType());
    if (ImGui::Combo("投影", &projType, projTypes, IM_ARRAYSIZE(projTypes))) {
        m_renderer->SerProjectionType(static_cast<ProjectionType>(projType));
    }
    
    ImGui::End();
}

void ToyEngineMainWindow::SelectObject(void* obj) {
    m_selectedObject = obj;
}

void ToyEngineMainWindow::UpdateSelectedObjectProperties() {
    if (m_selectedObjectType == "Model") {
        Model* model = static_cast<Model*>(m_selectedObject);
        ImGui::Text("模型名称: %s", model->GetName().c_str());
        
        // 位置属性
        glm::vec3 position = model->GetPosition();
        if (ImGui::DragFloat3("位置", glm::value_ptr(position), 0.1f)) {
            model->SetTranslate(position);
        }
        
        // 缩放属性
        glm::vec3 scale = model->GetScale();
        if (ImGui::DragFloat3("缩放", glm::value_ptr(scale), 0.1f)) {
            model->SetScale(scale);
        }
        
        // 旋转属性
        float rotation = model->GetRotation();
        if (ImGui::DragFloat("旋转", &rotation, 1.0f)) {
            model->SetRotate(rotation);
        }
        
    } else if (m_selectedObjectType == "Light") {
        auto light = static_cast<PointLight*>(m_selectedObject);
        ImGui::Text("光源名称: %s", light->GetName().c_str());
        
        // 光源颜色 - 使用ColorEdit3替代
        glm::vec3 color = light->Color;
        if (ImGui::ColorEdit3("颜色", glm::value_ptr(color))) {
            light->Color = color;
        }
        
        // 光源位置
        glm::vec3 position = light->Position;
        if (ImGui::DragFloat3("位置", glm::value_ptr(position), 0.1f)) {
            light->Position = position;
        }
        
    } else if (m_selectedObjectType == "Camera") {
        Camera* camera = static_cast<Camera*>(m_selectedObject);
        ImGui::Text("相机");
        
        // 相机位置
        glm::vec3 position = camera->GetEyePosition();
        if (ImGui::DragFloat3("位置", glm::value_ptr(position), 0.1f)) {
            // 更新相机位置
        }
    }
}

void ToyEngineMainWindow::OnMouseLeftButtonDown() {
    // 检查是否在ImGui区域内点击
    if (ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        return; // ImGui正在处理点击，不传递给3D场景
    }
    
    std::cout << "鼠标左键按下 - 位置: (" << m_currentMouseX << ", " << m_currentMouseY << ")" << std::endl;
    // 这里可以添加3D场景中的对象拾取逻辑
}

void ToyEngineMainWindow::OnMouseLeftButtonUp() {
    std::cout << "鼠标左键释放" << std::endl;
}

void ToyEngineMainWindow::OnMouseRightButtonDown() {
    if (ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        return;
    }
    
    m_cameraRotating = true;
    std::cout << "开始相机旋转" << std::endl;
}

void ToyEngineMainWindow::OnMouseRightButtonUp() {
    m_cameraRotating = false;
    std::cout << "结束相机旋转" << std::endl;
}

void ToyEngineMainWindow::OnMouseMove(double deltaX, double deltaY) {
    // 相机旋转控制
    if (m_cameraRotating && m_renderer) {
        auto camera = m_renderer->GetCamera();
        if (camera) {
            // 根据鼠标移动调整相机角度
            float sensitivity = 0.1f;
            camera->RotateHorizontal(static_cast<float>(-deltaX * sensitivity));
            camera->RotateVertical(static_cast<float>(-deltaY * sensitivity));
            std::cout << "相机旋转 - deltaX: " << deltaX << ", deltaY: " << deltaY << std::endl;
        }
    }
    
    // 相机平移控制（按住Shift键时）
    if (m_mouseLeftPressed && glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        m_cameraPanning = true;
        if (m_renderer) {
            auto camera = m_renderer->GetCamera();
            if (camera) {
                float panSpeed = 0.01f;
                camera->Pan(static_cast<float>(deltaX * panSpeed), static_cast<float>(-deltaY * panSpeed));
                std::cout << "相机平移 - deltaX: " << deltaX << ", deltaY: " << deltaY << std::endl;
            }
        }
    } else {
        m_cameraPanning = false;
    }
}

void ToyEngineMainWindow::OnMouseWheel(double delta) {
    if (m_renderer) {
        auto camera = m_renderer->GetCamera();
        if (camera) {
            // 相机缩放
            float zoomSpeed = 0.1f;
            camera->Zoom(static_cast<float>(delta * zoomSpeed));
            std::cout << "相机缩放 - delta: " << delta << std::endl;
        }
    }
}

// 静态回调函数实现
void ToyEngineMainWindow::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // 注意：由于这是静态函数，我们需要某种方式获取MainWindow实例
    // 这里简化处理，实际项目中可能需要使用glfwSetWindowUserPointer
    std::cout << "鼠标滚轮 - xoffset: " << xoffset << ", yoffset: " << yoffset << std::endl;
    // 在实际应用中，这里应该调用对应的MainWindow实例的方法
}

void ToyEngineMainWindow::Cleanup() {
    // 清理ImGui
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