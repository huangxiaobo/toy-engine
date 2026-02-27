#include <glad/gl.h>
#include <GLFW/glfw3.h>

// Dear ImGui
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>
#include "engine/mainwindow.h"

int main() {
    ToyEngineMainWindow mainWindow;
    
    if (!mainWindow.Initialize()) {
        std::cerr << "Failed to initialize main window" << std::endl;
        return -1;
    }
    
    mainWindow.Run();
    
    return 0;
}
