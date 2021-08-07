package engine

import (
	"fmt"
	_ "image/png"

	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/glfw/v3.2/glfw"
	"github.com/go-gl/mathgl/mgl32"

	"toy/engine/camera"
	"toy/engine/config"
	"toy/engine/light"
	"toy/engine/logger"
	"toy/engine/text"
)

type World struct {
	window     *glfw.Window
	Light      *light.PointLight
	renderObjs []Mesh
	Camera     *camera.Camera
	Text       *text.Text

	bRun bool
}

func (w *World) initGLFW() {

	var err error = nil
	if err := glfw.Init(); err != nil {
		logger.Fatal("failed to initialize glfw:", err)
	}

	previousTime := glfw.GetTime()
	logger.Info(previousTime)

	glfw.WindowHint(glfw.Resizable, glfw.False)
	glfw.WindowHint(glfw.ContextVersionMajor, 4)
	glfw.WindowHint(glfw.ContextVersionMinor, 1)
	glfw.WindowHint(glfw.OpenGLProfile, glfw.OpenGLCoreProfile)
	glfw.WindowHint(glfw.OpenGLForwardCompatible, glfw.True)
	w.window, err = glfw.CreateWindow(config.Config.WindowWidth, config.Config.WindowHeight, "Toy引擎", nil, nil)
	if err != nil {
		panic(err)
	}
	w.window.MakeContextCurrent()

	w.window.SetKeyCallback(func(window *glfw.Window, key glfw.Key, scancode int, action glfw.Action, mods glfw.ModifierKey) {
		fmt.Println(key)
		switch key {
		case glfw.KeyEscape:
			w.bRun = false

		}
	})

}

func (w *World) initGL() {
	// Initialize Glow
	if err := gl.Init(); err != nil {
		panic(err)
	}

	version := gl.GoStr(gl.GetString(gl.VERSION))
	logger.Info("OpenGL version", version)

	// Configure global settings
	gl.Enable(gl.DEPTH_TEST)
	gl.DepthFunc(gl.LESS)
	gl.ClearColor(1.0, 1.0, 1.0, 1.0)

	gl.Enable(gl.SAMPLES)
	glfw.WindowHint(glfw.Samples, 8)

	// 只显示正面 , 不显示背面
	// gl.Enable(gl.CULL_FACE)

	// 设置顺时针方向 CW : Clock Wind 顺时针方向
	// 默认是 GL_CCW : Counter Clock Wind 逆时针方向
	gl.FrontFace(gl.CCW)

	// 设置线框模式
	// 设置了该模式后 , 之后的所有图形都会变成线
	// gl.PolygonMode(gl.FRONT, gl.LINE)

	// 设置点模式
	// 设置了该模式后 , 之后的所有图形都会变成点
	// glPolygonMode(GL_FRONT, GL_POINT);

}

func (w *World) Init() error {

	w.initGLFW()
	w.initGL()

	// 初始化摄像机
	w.Camera = new(camera.Camera)
	w.Camera.Init(mgl32.Vec3{10.0, 10.0, 10.0}, mgl32.Vec3{-10.0, -10.0, -10.0})

	// 初始化灯光
	w.Light = &light.PointLight{Position: mgl32.Vec4{0, 5.0, 0, 0.0}, Color: mgl32.Vec3{0.02, 0.1, 0.1}}
	w.Light.Init()

	// Text
	w.Text = &text.Text{}
	w.Text.Init()
	w.Text.Load("./resource/font/微软雅黑.ttf", 16)

	w.bRun = true
	return nil
}

func (w *World) Destroy() {

	glfw.Terminate()
}

func (w *World) Run() {
	previousTime := glfw.GetTime()
	for !w.window.ShouldClose() {
		gl.ClearColor(0.0, 0.0, 0.0, 0.0)
		gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

		w.DrawAxis()
		w.DrawLight()
		// Update
		time := glfw.GetTime()
		elapsed := time - previousTime
		previousTime = time

		for _, renderObj := range w.renderObjs {
			renderObj.Update(elapsed)
			renderObj.Render(w)
		}

		// 字体
		w.Text.Render(0, 50, mgl32.Vec4{1.0, 1.0, 1.0, 1.0})

		// Maintenance
		w.window.SwapBuffers()
		glfw.PollEvents()

		if !w.bRun {
			break
		}
	}
}

func (w *World) DrawAxis() {
	// logger.Info("DrawAxis...")
}

func (w *World) DrawLight() {
	// Render
	width := float32(config.Config.WindowWidth)
	height := float32(config.Config.WindowHeight)
	projection := mgl32.Perspective(
		mgl32.DegToRad(w.Camera.Zoom),
		width/height,
		0.1,
		100.0,
	)
	view := w.Camera.GetViewMatrix()

	model := mgl32.Ident4().Mul4(mgl32.Scale3D(1, 1, 1))

	position := w.Light.Position
	model = model.Add(mgl32.Translate3D(position.X(), position.Y(), position.Z()))

	w.Light.Render(projection, view, model)

}

func (w *World) AddRenderObj(renderObj Mesh) {
	w.renderObjs = append(w.renderObjs, renderObj)
	renderObj.Init(w)
}
