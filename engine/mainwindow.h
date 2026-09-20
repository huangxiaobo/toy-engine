#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <vector>
#include <map>

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
    ToyEngineMainWindow();
    ~ToyEngineMainWindow();

    bool Initialize();
    void Run();
    void RenderFrame();
    void ProcessInput();
    void Cleanup();

    GLFWwindow* GetWindow() const { return m_window; }
    Renderer* GetRenderer() const { return m_renderer.get(); }

private:
    // ---- ImGui 面板创建 ----
    void CreateDockSpace();                  // 创建 DockSpace 并建立初始三栏布局，同时记录中央节点（3D视口）矩形
    void CreateMenuBar();                    // 渲染主菜单栏（退出 / 面板子菜单含资源、属性开关），位于 DockSpace 宿主窗口顶部
    void CreateUI();
    void CreateResourceListPanel();
    void CreatePropertiesPanel();
    void ShowViewportStatusBar();
    void DrawViewportAxisGizmo();            // 在视口左下角叠加屏幕空间坐标轴 gizmo（三色六轴 + X/Y/Z 标签）
    void ShowShadowDepthMapPanel();          // 绘制阴影深度贴图可视化调试面板（把深度图作为纹理显示）
    void ShowShadowPropertiesPanel();        // 绘制阴影属性面板（阴影开关、深度贴图预览等全局阴影设置）
    void ShowDebugPropertiesPanel();         // 绘制调试属性面板（DebugDraw 光源线框开关等渲染调试项）
    void ShowFpsGraph();                     // 绘制视口右上角 FPS 曲线悬浮面板（最近一帧一次的瞬时帧率采样）
    void RebuildShadowDepthPreview(unsigned int srcTex); // 读回深度贴图并重建灰度预览纹理
    void SaveScreenshot();                   // 截图：读取当前默认framebuffer并保存为 PNG 文件（含时间戳文件名）

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

    // ---- 3D 拾取（左键点击无拖拽时触发，屏幕射线 → 逐对象求交 → 最近命中 → SelectObject）----
    void PerformPick();

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
    std::unique_ptr<Renderer> m_renderer;

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
    // FPS 曲线悬浮面板开关（菜单「面板 → FPS曲线」控制，默认显示）
    bool m_showFpsGraph = true;
    // 阴影深度贴图可视化调试面板开关（把深度图作为纹理显示，辅助诊断阴影问题）
    bool m_showShadowDepthMap = false;
    // 阴影属性面板开关（阴影开关、光源摄像机参数、深度贴图预览等全局阴影设置）
    // 默认显示：阴影参数属于常用调试信息，启动即展示，无需先通过菜单打开
    bool m_showShadowProperties = true;
    // 调试属性面板开关（DebugDraw 光源线框开关等渲染调试项）
    bool m_showDebugProperties = true;

    // 阴影深度贴图可视化面板的显示状态：
    // 深度贴图是 GL_DEPTH_COMPONENT 只写格式，ImGui 颜色采样器难以可靠显示，
    // 故每次面板打开时把深度读回 CPU、归一化为灰度并上传到一张 RGBA8 纹理再展示。
    unsigned int m_shadowDepthPreviewTex = 0;   // 承载归一化灰度预览的 RGBA8 纹理（缓存复用）
    int m_shadowDepthPreviewW = 0;              // 预览纹理宽度（读回分辨率）
    int m_shadowDepthPreviewH = 0;              // 预览纹理高度（读回分辨率）
    unsigned int m_lastDisplayedDepthTex = 0;   // 上次显示的源深度贴图 ID，用于缓存失效判断

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
    // 左键按下时的光标位置（松开时用于区分"点击"与"拖拽"，位移小于阈值视为点击并触发拾取）
    double m_mouseDownX = 0.0;
    double m_mouseDownY = 0.0;

    // 相机控制状态
    bool m_cameraPanning = false;
    int m_currentCameraIndex = 0;

    // 时间相关
    float m_lastTime = 0.0f;
    float m_deltaTime = 0.0f;

    // FPS 曲线历史采样：以 1/15 秒为周期结算一次窗口平均帧率入历史
    // （容量到达 kFpsHistoryCapacity 后丢弃最旧采样，vector 前移开销可忽略）
    std::vector<float> m_fps_history;
    // 采样累加器：跨到结算间隔（1/15 秒）时计算帧数/流逝时间写入 m_fps_history 并清零
    unsigned int m_fps_sample_frames = 0;
    float m_fps_sample_elapsed = 0.0f;

    // 截图请求标志：菜单「工具 → 截图」置位，RenderFrame 在本帧渲染完成后、
    // swapBuffers 之前执行一次截图并复位（保证截取的是完整一帧，含 UI 叠加）
    bool m_screenshotPending = false;
};

#endif
