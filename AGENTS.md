# 程序的编译
```
cmake -E make_directory build
cmake -E chdir build cmake ..

```

# 程序的运行
```
./build/bin/toy-engine
```

# 必须遵守
- 实现功能的时候，必须添加详细的注释

# 经验教训

## 1. 初始化顺序问题（空指针段错误）

**问题**：`Init()` 创建 chunk 时，`m_technique` 还未设置，导致生成的 mesh 没有 `m_effect`。之后 `SetTechnique()` 只更新了 chunk 级别的指针，但已生成的 mesh 没有同步。`Mesh::Draw()` 调用 `m_effect->Enable()` 时空指针解引用 → 段错误。

**修复**：`TerrainChunk::SetTechnique()` 和 `SetTexture()` 必须遍历所有已生成的 mesh，同步更新它们的 `m_effect` 和 `m_textureID`。

**规则**：
- 纹理/着色器等资源的绑定必须考虑「先创建后绑定」的情况
- setter 方法如果影响已创建的子对象，必须同步更新所有子对象
- 创建 OpenGL 资源（mesh、纹理）时，必须确保所有依赖已就绪

## 2. GL状态泄漏导致渲染异常

**问题**：`ParticleSystem::Draw()` 无条件调用 `glEnable(GL_CULL_FACE)`，导致后续 `SkyDome::Draw()` 时背面剔除未关闭，天空球不可见。

**修复**：绘制前后保存/恢复 OpenGL 状态。

**规则**：
- 修改全局 OpenGL 状态（如 `GL_CULL_FACE`、`GL_BLEND`）时，必须保存和恢复
- 不要假设其他模块会正确设置 GL 状态

## 3. 摄像机位置与 chunk 加载

**问题**：`m_lastCameraChunk` 初始化为 (0,0)，与 `Init()` 时加载的 chunk 坐标相同，导致 `Update()` 认为摄像机未移动，跳过加载。

**修复**：在 `Init()` 中预加载初始 chunks，并正确设置 `m_lastCameraChunk`。

**规则**：
- 避免使用「默认值」作为跳过更新的判断条件
- 初始化时必须完成首帧所需的所有数据加载

## 4. GLSL 字符串终止符

**问题**：着色器代码字符串缺少 `\000` 后缀导致编译失败。

**规则**：GLSL 着色器字符串必须以 `\000` 结尾

## 5. 头文件 include 顺序

**问题**：`#include <glad/glad.h>` 必须放在所有头文件之前，否则报错 "OpenGL header already included"。

**规则**：`glad/glad.h` 必须是第一个 include