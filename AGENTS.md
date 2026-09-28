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
- 实现功能的时候，必须添加简要的注释,注释避免AI味道
- 变量命名统一使用 snake_case（如 `m_shadow_map_ready`、`m_clear_color`），不使用 camelCase（如 `m_shadowMapReady`、`m_clearColor`）
  - **所有类成员变量必须以 `m_` 开头** + snake_case（如 `m_texture_id`、`m_depth_rbo`、`m_last_time`），不要写 `m_textureID`、`m_depthRbo`、`m_lastTime`
  - 成员变量前缀规则不适用于：局部变量、函数参数、静态常量/枚举值、类型名、GLSL 结构体字段名
  - 提交前自查，下面这条命令应无输出（检查 `m_` 后仍带 camelCase 的违规命名）：
    ```
    grep -rE '\bm_[a-z0-9]+[A-Z]' engine/ --include='*.h' --include='*.cpp'
    ```
  - 注意：grep 无法可靠检查「漏写 `m_` 前缀的类成员」（方法签名、局部变量、静态常量都会误报），此类检查需用脚本抽取类成员逐个核对

# 经验教训

## 1. 初始化顺序问题（空指针段错误）

**问题**：`Init()` 创建 chunk 时，`m_technique` 还未设置，导致生成的 mesh 没有 `m_effect`。之后 `SetTechnique()` 只更新了 chunk 级别的指针，但已生成的 mesh 没有同步。`Mesh::Draw()` 调用 `m_effect->Enable()` 时空指针解引用 → 段错误。

**修复**：`TerrainChunk::SetTechnique()` 和 `SetTexture()` 必须遍历所有已生成的 mesh，同步更新它们的 `m_effect` 和 `m_texture_id`。

**规则**：
- 纹理/着色器等资源的绑定必须考虑「先创建后绑定」的情况
- setter 方法如果影响已创建的子对象，必须同步更新所有子对象
- 创建 OpenGL 资源（mesh、纹理）时，必须确保所有依赖已就绪

## 2. GL状态泄漏导致渲染异常

**问题**：`ParticleSystem::Draw()` 无条件调用 `glEnable(GL_CULL_FACE)`，导致后续 `SkyDome::Draw()` 时背面剔除未关闭，天空球不可见。

**修复**：改用 `engine/utils/gl_state_guard.h` 里的 RAII guard，在绘制作用域内声明、随作用域结束自动恢复，不再手写 `glGet*` + 条件恢复。

**规则**：
- 修改全局 OpenGL 状态时，在作用域内声明对应的 `GLStateGuard` 派生 guard，不要手写保存/恢复
- 一个状态若有"开关 + 参数"两部分（如 `GL_POLYGON_OFFSET_FILL` 与 `glPolygonOffset` 的 factor/units、`GL_BLEND` 与 `glBlendFunc`），guard 必须把参数一并恢复，只恢复开关仍会改变渲染结果
- guard 的作用域必须紧贴状态生效区间；跨到下一个绘制 Pass 还没释放，会让后处理等后续绘制继承错误状态
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

## 6. 批量改名时 LSP 可能漏改调用点

**问题**：`Mesh::m_textureID` 用 LSP rename 改名时，只改了 `mesh.h` 里的 3 处，`mesh.cpp` 的 3 处调用点没改到，编译直接失败；再对同一符号发起 rename，clangd 反而报 "new name is the same as the old name"——它的索引已经和磁盘内容不一致了。

**规则**：
- 批量改名不要只依赖 LSP rename，改完必须 grep 确认旧名字在工程里已无残留（注释里的引用也算）
- 改名后必须完整编译，编译结果是唯一可信的验证手段
- 同一个名字分布在多个类时（如 `m_textureID` 同时存在于 Mesh/TerrainChunk/TerrainManager/ParticleSystem），先确认各类的目标名一致才能做整体替换，否则按类分别改名
- 公有成员（PascalCase，如 `Light::Color`）与局部变量、函数形参同名时不能做无差别文本替换，必须逐个类用 LSP 改名或人工改，并逐类编译

## 7. 批量改名误改 GLSL uniform 字符串导致光照静默失效

**问题**：全量成员改名时，把 `technique_light.cpp` 里的 uniform 位置字符串也从 PascalCase 改成了 snake_case（`"gDirectionLight.Direction"` → `"gDirectionLight.direction"`），但 lit/terrain/toon.frag 的结构体字段仍是 PascalCase。`glGetUniformLocation` 找不到名字返回 -1，`Shader::GetUniformLocation()` 返回的 unsigned int 变成全 1 位模式，`glUniform*` 按规范**静默忽略**——所有灯光/材质 uniform 全部落空，光照变暗但不报错、不崩溃、编译也通过。

**修复**：把 30 处 uniform 字符串恢复为与 GLSL struct 字段一致的 PascalCase（`gDirectionLight.Direction`、`gMaterial.AmbientColor`、`gPointLights[i].Color` 等）。

**规则**：
- 字符串字面量里的 GLSL uniform 名是「跨语言的契约」，与 `.frag`/`.vert` 里的 struct 字段一一对应，改名时必须整体排除，不能跟随 C++ 成员命名规则
- 批量改名后除了编译，还要对 uniform 字符串与 shader 做一次交叉核对（grep 每个字符串在 shader 里是否存在）
- uniform 找不到通常静默失败（返回 -1 被当成无符号全 1），**编译通过 + 不崩溃 ≠ 渲染正确**，视觉回归要靠实际运行确认

