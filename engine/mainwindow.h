#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <map>

using namespace std;

class Renderer;
class Model;
class Light;

/*
 * 主窗口类（ToyEngineMainWindow）
 *
 * 负责创建 GLFW 窗口、初始化 OpenGL 和 ImGui，以及管理基于 ImGui
 * DockSpace 的可停靠编辑器布局：
 *   - 左侧停靠：资源列表（来自 world.yaml 的所有资源：摄像机、灯光、模型、地形、天空穹、粒子）
 *   - 中央节点：3D 渲染视口（PassthruCentralNode——场景直接绘制在 GL 视口上，
 *               中央节点不创建 ImGui 窗口，鼠标输入可穿透用于相机控制）
 *   - 右侧停靠：属性面板（显示选中资源的所有可编辑属性）
 *
 * 与旧版「固定三栏 + 自绘分隔条」不同，DockSpace 布局允许用户自由拖拽、
 * 浮动、调整各面板大小，并可通过 DockBuilder 在首帧建立默认布局。
 * 选择模型：点击左侧资源列表中的任意项，右侧面板即显示该项的全部属性。
 */
class ToyEngineMainWindow  {

public:
    ToyEngineMainWindow() = default;
    ~ToyEngineMainWindow();

    bool Initialize();
    void Run();
    void RenderFrame();
    void ProcessInput();
    void Cleanup();

    GLFWwindow* GetWindow() const { return m_window; }
    Renderer* GetRenderer() const { return m_renderer; }

private:
    // ---- ImGui 面板创建 ----
    void CreateDockSpace();                  // 创建 DockSpace 并建立初始三栏布局，同时记录中央节点（3D视口）矩形
    void CreateMenuBar();                    // 渲染主菜单栏（退出 / 面板子菜单含资源、属性开关），位于 DockSpace 宿主窗口顶部
    void CreateUI();
    void CreateResourceListPanel();
    void CreatePropertiesPanel();
    void ShowViewportStatusBar();

    // ---- 各资源类型的属性编辑器 ----
    void ShowModelProperties();
    void ShowLightProperties();
    void ShowCameraProperties();
    void ShowTerrainProperties();
    void ShowSkyDomeProperties();
    void ShowParticleProperties();

    // ---- 选择管理 ----
    void SelectObject(void* obj, const std::string& type);
    void ClearSelection();

    // ---- 鼠标事件处理 ----
    void OnMouseLeftButtonDown();
    void OnMouseLeftButtonUp();
    void OnMouseRightButtonDown();
    void OnMouseRightButtonUp();
    void OnMouseMove(double deltaX, double deltaY);
    void OnMouseWheel(double delta);

    // ---- GLFW 静态回调（转发到实例方法） ----
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    GLFWwindow* m_window = nullptr;
    Renderer* m_renderer = nullptr;

    // 窗口尺寸
    int m_windowWidth = 1280;
    int m_windowHeight = 720;

    // 中央节点（3D 渲染视口）矩形，单位为 ImGui 窗口逻辑坐标（左上角为原点）
    // 每帧由 CreateDockSpace() 更新，RenderFrame() 据此设置 glViewport
    float m_viewportX = 0.0f;
    float m_viewportY = 0.0f;
    float m_viewportWidth = 0.0f;
    float m_viewportHeight = 0.0f;

    // DockSpace 是否已完成初始布局（避免每帧重复执行 DockBuilder）
    bool m_dockspaceInitialized = false;

    // 面板显示控制
    bool m_showResourceList = true;
    bool m_showProperties = true;
    bool m_showViewportStatusBar = true;

    // 当前选中的资源（union 风格：指针 + 类型字符串）
    // 支持类型："Model", "Light", "Camera", "Terrain", "SkyDome", "Particle"
    void* m_selectedObject = nullptr;
    std::string m_selectedObjectType;
    int m_selectedParticleIndex = -1;   // 粒子系统选中索引（粒子用索引而非指针）

    // 鼠标状态
    bool m_mouseLeftPressed = false;
    bool m_mouseRightPressed = false;
    double m_currentMouseX = 0.0;
    double m_currentMouseY = 0.0;

    // 相机控制状态
    bool m_cameraPanning = false;
    int m_currentCameraIndex = 0;

    // 时间相关
    float m_lastTime = 0.0f;
    float m_deltaTime = 0.0f;
};

#endif
