# 第三方库管理

## 目录结构

```
3rdparty/
├── glad/          # OpenGL加载库 (本地)
├── glfw/          # OpenGL窗口库 (需下载)
├── imgui/         # GUI库 (需下载)
├── assimp/        # 3D模型加载库 (需下载)
├── yaml-cpp/      # YAML解析库 (需下载)
└── stb/           # 图像加载库 (需下载)
```

## 首次使用

在首次编译前,需要下载第三方库:

```bash
./download_deps.sh
```

这个脚本会自动下载所有必需的第三方库到 `3rdparty/` 目录。

## 编译优化说明

### 优势

1. **避免重复编译**: 使用本地3rdparty目录后,CMake会缓存编译结果,只在库源码变化时才重新编译
2. **离线编译**: 下载一次后,可以离线编译项目
3. **版本控制**: 可以在.gitignore中排除3rdparty目录,但记录版本号
4. **构建速度**: 第二次及以后的编译只编译项目代码,不编译第三方库

### CMake缓存机制

- CMake会在 `build/` 目录下缓存第三方库的编译结果
- 只有当以下情况发生时才会重新编译第三方库:
  - 删除了 `build/` 目录
  - 修改了 `3rdparty/` 中的库源码
  - 运行了 `cmake ..` 并检测到配置变化

### 日常开发

正常开发时,只需要:

```bash
cd build
cmake ..
make -j8
```

CMake会自动检测并使用已编译的第三方库,不会重复编译。

## 更新第三方库

如果需要更新某个库的版本:

```bash
cd 3rdparty/<library-name>
git pull
git checkout <new-version>
```

然后重新编译项目即可。

## 注意事项

1. `3rdparty/` 目录较大,建议添加到 `.gitignore`
2. 建议在项目中记录各库的版本号,方便团队协作
3. 可以使用git submodule来管理第三方库(可选)
