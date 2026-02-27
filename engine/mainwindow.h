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

class ToyEngineMainWindow  {

public:
    ToyEngineMainWindow() = default;
    ~ToyEngineMainWindow();

    // 初始化窗口
    bool Initialize();
    
    // 主循环
    void Run();
    
    // 渲染一帧
    void RenderFrame();
    
    // 处理输入
    void ProcessInput();
    
    // 清理资源
    void Cleanup();
    
    // 获取窗口句柄
    GLFWwindow* GetWindow() const { return m_window; }
    
    // 获取渲染器
    Renderer* GetRenderer() const { return m_renderer; }

private:
    // 创建ImGui界面
    void CreateUI();
    
    // 创建场景树面板
    void CreateSceneTreePanel();
    
    // 创建属性面板
    void CreatePropertiesPanel();
    
    // 创建工具栏
    void CreateToolbar();
    
    // 创建状态栏
    void CreateStatusBar();
    
    // 选择对象
    void SelectObject(void* obj);
    
    // 更新选中对象的属性
    void UpdateSelectedObjectProperties();
    
    // 鼠标事件处理
    void OnMouseLeftButtonDown();
    void OnMouseLeftButtonUp();
    void OnMouseRightButtonDown();
    void OnMouseRightButtonUp();
    void OnMouseMove(double deltaX, double deltaY);
    void OnMouseWheel(double delta);
    
    // GLFW回调函数
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
    GLFWwindow* m_window = nullptr;
    Renderer* m_renderer = nullptr;
    
    // 窗口尺寸
    int m_windowWidth = 1280;
    int m_windowHeight = 720;
    
    // UI状态
    bool m_showSceneTree = true;
    bool m_showProperties = true;
    bool m_showToolbar = true;
    bool m_showStatusBar = true;
    
    // 选中的对象
    void* m_selectedObject = nullptr;
    std::string m_selectedObjectType;
    
    // 鼠标状态
    bool m_mouseLeftPressed = false;
    bool m_mouseRightPressed = false;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    double m_currentMouseX = 0.0;
    double m_currentMouseY = 0.0;
    
    // 相机控制状态
    bool m_cameraRotating = false;
    bool m_cameraPanning = false;
    
    // 时间相关
    float m_lastTime = 0.0f;
    float m_deltaTime = 0.0f;
};

#endif
