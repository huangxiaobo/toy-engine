### 依赖

> https://github.com/JoeyDeVries/LearnOpenGL

> SD2
http://libsdl.org/download-2.0.php

#### gcc
https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.1.0/threads-win32/seh/x86_64-8.1.0-release-win32-seh-rt_v6-rev0.7z

### Ubuntu安装依赖

```
sudo apt install assimp-utils libassimp-dev
sudo apt install build-essential
sudo apt install libx11-dev
sudo apt install xorg-dev
```

### 参考

> https://www.opengl.org/archives/resources/code/samples/glut_examples/examples/examples.html
> https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/7.4.camera_class/camera_class.cpp
> https://ogldev.org/index.html

obj格式
> https://www.cs.cmu.edu/~mbz/personal/graphics/obj.html

### 问题解决记录

####

```shader
panic: failed to compile #version 330

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

in vec3 FragPos;
in vec3 Normal;
out vec4 color;
void main() {
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    color = vec4(diffuse, 1.0);
}
: 0:17(1): error: syntax error, unexpected $undefined, expecting $end

```

uniform前面不要有空行 有空行

## 坐标系

OpenGL是右手坐标系

## 字体

https://stackoverflow.com/questions/34455925/render-truetype-fonts-with-opengl
https://github.com/golang/freetype/blob/master/example/freetype/main.go

### freetype

https://developer.apple.com/library/archive/documentation/TextFonts/Conceptual/CocoaTextArchitecture/Art/glyph_metrics_2x.png
https://developer.apple.com/library/archive/documentation/TextFonts/Conceptual/CocoaTextArchitecture/Art/glyph_metrics_2x.png

重要的字体度量参数：

- 上下高度 (ascent) ： 从基线到放置轮廓点最高 ( 上 ) 的距离；
- 下行高度 (descent) ：从基线到放置轮廓点最低 ( 下 ) 的距离；
- 左跨距 ( bearingX ) ： 从当前笔位置到轮廓左边界的水平位置；
- 上跨距 ( bearingY ) ： 从当前笔位置到轮廓上边界的垂直位置；
- 步进宽度 ( advanceX ): 相邻两个笔位置的水平距离 ( 字间距 )；
- 字形宽度 (width) ： 字形的水平长度；
- 字形高度 (height) ： 字形的垂直长度。

## ScreenShots
![屏幕截圖](https://github.com/huangxiaobo/ToyEngine/blob/master/screenshots/Screenshot%20from%202021-06-26%2017-22-03.png)

## 网格球体
> http://www.songho.ca/opengl/gl_sphere.html