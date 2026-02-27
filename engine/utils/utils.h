#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <glm/glm.hpp>

class Utils {
public:
    static void DebugMatrix(const glm::mat4 &mat);

    static std::string GetString(const glm::vec3 vec);

    static std::string GetString(const glm::vec2 vec);

    static std::string GenerateUUID();

    static void PrintStackTrace();

    // 从文件加载纹理并返回OpenGL纹理ID
    static unsigned int LoadTextureFromFile(const std::string &path);
    
    // 创建棋盘格纹理
    static unsigned int CreateCheckerboardTexture(int width, int height, int checkSize);
};

#endif
