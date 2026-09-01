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
 * 负责创建 GLFW 窗口、初始化 OpenGL 和 ImGui，以及管理三栏式编辑器布局：
 *   - 左栏：资源列表（来自 world.yaml 的所有资源：摄像机、灯光、模型、地形、天空穹、粒子）
 *   - 中栏：3D 渲染视口
 *   - 右栏：属性面板（显示选中资源的所有可编辑属性）
 *
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
    void CreateUI();
    void CreateResourceListPanel();
    void CreatePropertiesPanel();
    void ShowViewportStatusBar();
    void CreateSplitter(float splitterPosX, bool& active, float& panelWidth, float minWidth, float maxWidth);

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

    // 三栏布局面板宽度（支持拖动调整）
    float m_leftPanelWidth = 250.0f;
    float m_rightPanelWidth = 300.0f;
    // 分隔条拖动状态
    bool m_draggingLeftSplitter = false;
    bool m_draggingRightSplitter = false;
    static constexpr float SPLITTER_WIDTH = 5.0f;   // 分隔条厚度
    static constexpr float MIN_PANEL_WIDTH = 150.0f; // 面板最小宽度

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
