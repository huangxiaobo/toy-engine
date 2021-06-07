package engine

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/glfw/v3.2/glfw"
	"github.com/go-gl/mathgl/mgl32"
	_ "image/png"
	"toy/engine/camera"
	"toy/engine/light"
	"toy/engine/logger"
)

const (
	WindowWidth  = 800
	WindowHeight = 600
)

type World struct {
	window     *glfw.Window
	Light      *light.Light
	renderObjs []Mesh
	Camera     *camera.Camera
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
	w.window, err = glfw.CreateWindow(WindowWidth, WindowHeight, "Cube", nil, nil)
	if err != nil {
		panic(err)
	}
	w.window.MakeContextCurrent()

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
}

func (w *World) Init() error {

	w.initGLFW()
	w.initGL()

	// 初始化摄像机
	w.Camera = new(camera.Camera)
	w.Camera.Init(mgl32.Vec3{10.0, 10.0, 10.0}, mgl32.Vec3{-10.0, -10.0, -10.0})

	// 初始化灯光
	w.Light = &light.Light{Position: mgl32.Vec3{0.0, 0.0, 10.0}, Color: mgl32.Vec3{0.8, 0.8, 0.8}}

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

		// Update
		time := glfw.GetTime()
		elapsed := time - previousTime
		previousTime = time

		for _, renderObj := range w.renderObjs {
			renderObj.Update(elapsed)
			renderObj.Render(w)
		}

		// Maintenance
		w.window.SwapBuffers()
		glfw.PollEvents()
	}
}

func (w *World) DrawAxis() {
	//logger.Info("DrawAxis...")
}

func (w *World) AddRenderObj(renderObj Mesh) {
	w.renderObjs = append(w.renderObjs, renderObj)
	renderObj.Init(w)
}
