#version 330 core

/*
 * 粒子系统 - 顶点着色器
 *
 * 功能：
 *   1. 将粒子从模型空间变换到裁剪空间（MVP 变换）
 *   2. 传递颜色和生命比例到片段着色器
 *   3. 使用 gl_PointSize 动态控制点精灵大小
 *
 * 渲染方式：GL_POINTS（点精灵）
 *   - 每个粒子渲染为一个正方形面片（点精灵）
 *   - 面片大小由 gl_PointSize 控制，单位为像素
 *   - gl_PointCoord 提供面片内的 [0,1] 纹理坐标
 *   - 片段着色器中可利用 gl_PointCoord 实现圆形裁剪等效果
 *
 * 顶点属性（由 CPU 端 VBO 提供）：
 *   location 0: position  - 粒子在模型空间的位置
 *   location 1: color     - 粒子当前颜色（由 CPU 端插值计算）
 *   location 2: size      - 粒子基础大小（像素）
 *   location 3: life      - 粒子剩余生命（秒）
 *   location 4: maxLife   - 粒子最大生命（秒）
 */

// --- 顶点属性 ---
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in float size;
layout (location = 3) in float life;
layout (location = 4) in float maxLife;

// --- Uniform：模型-视图-投影矩阵 ---
uniform mat4 projection;  // 投影矩阵（透视/正交）
uniform mat4 view;        // 视图矩阵（摄像机变换）
uniform mat4 model;       // 模型矩阵（世界变换，粒子系统整体位移/旋转/缩放）

// --- 输出到片段着色器 ---
out vec3 vColor;       // 粒子颜色
out float vLifeRatio;  // 生命比例 = life / maxLife，范围 [0, 1]
                       //   1.0 = 刚出生（满血）
                       //   0.0 = 即将死亡

void main() {
    // MVP 变换：模型空间 → 世界空间 → 观察空间 → 裁剪空间
    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;

    // 生命比例：用于片段着色器中的透明度渐变和大小缩放
    vLifeRatio = life / maxLife;
    vColor = color;

    // 点精灵大小 = 基础大小 × 衰减系数
    //   随生命减少（vLifeRatio → 0），粒子逐渐膨胀（最大 1.5 倍）
    //   营造烟花粒子"爆开后扩散"的视觉效果
    gl_PointSize = size * (1.0 + (1.0 - vLifeRatio) * 0.5);
}
