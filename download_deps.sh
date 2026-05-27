#!/bin/bash

# 第三方库下载脚本
# 用法: ./download_deps.sh

set -e

THIRD_PARTY_DIR="$(cd "$(dirname "$0")" && pwd)/3rdparty"

echo "开始下载第三方库到 $THIRD_PARTY_DIR ..."

# 下载 GLFW
if [ ! -d "$THIRD_PARTY_DIR/glfw/.git" ]; then
    echo "下载 GLFW..."
    git clone https://gitcode.com/GitHub_Trending/gl/glfw.git "$THIRD_PARTY_DIR/glfw"
    cd "$THIRD_PARTY_DIR/glfw"
    git checkout 3.4
    cd ..
else
    echo "GLFW 已存在,跳过"
fi

# 下载 ImGui
if [ ! -d "$THIRD_PARTY_DIR/imgui/.git" ]; then
    echo "下载 ImGui..."
    git clone git@gitee.com:mirrors/imgui.git "$THIRD_PARTY_DIR/imgui"
    cd "$THIRD_PARTY_DIR/imgui"
    git checkout v1.91.0
    cd ..
else
    echo "ImGui 已存在,跳过"
fi

# 下载 Assimp
if [ ! -d "$THIRD_PARTY_DIR/assimp/.git" ]; then
    echo "下载 Assimp..."
    git clone git@gitee.com:mirrors/assimp.git "$THIRD_PARTY_DIR/assimp"
    cd "$THIRD_PARTY_DIR/assimp"
    git checkout v5.4.2
    cd ..
else
    echo "Assimp 已存在,跳过"
fi

# 下载 yaml-cpp
if [ ! -d "$THIRD_PARTY_DIR/yaml-cpp/.git" ]; then
    echo "下载 yaml-cpp..."
    git clone https://gitcode.com/gh_mirrors/ya/yaml-cpp.git "$THIRD_PARTY_DIR/yaml-cpp"
    cd "$THIRD_PARTY_DIR/yaml-cpp"
    git checkout yaml-cpp-0.9.0
    cd ..
else
    echo "yaml-cpp 已存在,跳过"
fi

# 下载 stb
if [ ! -d "$THIRD_PARTY_DIR/stb/.git" ]; then
    echo "下载 stb..."
    git clone git@gitee.com:mirrors/stb.git "$THIRD_PARTY_DIR/stb"
    cd "$THIRD_PARTY_DIR/stb"
    git checkout master
    cd ..
else
    echo "stb 已存在,跳过"
fi

echo "所有第三方库下载完成!"
echo "目录结构:"
ls -la "$THIRD_PARTY_DIR"
