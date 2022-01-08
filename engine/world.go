package engine

import (
	"github.com/huangxiaobo/toy-engine/engine/model"
	"github.com/huangxiaobo/toy-engine/engine/platforms"
	"github.com/huangxiaobo/toy-engine/engine/text"
	_ "image/png"

	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"

	"github.com/huangxiaobo/toy-engine/engine/camera"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/light"
	"github.com/huangxiaobo/toy-engine/engine/logger"
)

type World struct {
	platform   *platforms.SDL
	Light      *light.PointLight
	renderObjs []model.RenderObj
	Camera     *camera.Camera
	Text       *text.Text

	bRun bool
}

func (w *World) initSDL() {
	var err error

	windowWidth := config.Config.WindowWidth
	windowHeight := config.Config.WindowHeight
	w.platform, err = platforms.NewSDL(platforms.SDLClientAPIOpenGL4, windowWidth, windowHeight)
	if err != nil {
		panic(err)
	}

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

	w.initSDL()
	w.initGL()

	// 初始化摄像机
	w.Camera = new(camera.Camera)
	w.Camera.Init(mgl32.Vec3{0.0, 50.0, 50.0}, mgl32.Vec3{-0.0, -0.0, -0.0})

	// 初始化灯光
	w.Light = &light.PointLight{Position: mgl32.Vec4{0, 10.0, 0, 0.0}, Color: mgl32.Vec3{1.0, 1.0, 1.0}}
	w.Light.Init()

	// Text
	w.Text, _ = text.NewText("Toy引擎", 32)

	w.bRun = true
	return nil
}

func (w *World) Destroy() {
	w.platform.Dispose()
}

func (w *World) Run() {

	for !w.platform.ShouldStop() {
		w.platform.ProcessEvents()

		gl.ClearColor(0.8, 0.85, 0.85, 0.0)
		gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

		projection := mgl32.Perspective(
			mgl32.DegToRad(w.Camera.Zoom),
			float32(config.Config.WindowHeight/config.Config.WindowHeight),
			0.1,
			100.0,
		)
		view := w.Camera.GetViewMatrix()
		model := mgl32.Ident4()
		//mvp := projection.Mul4(view).Mul4(model)

		w.DrawAxis()
		w.DrawLight()
		// Update
		elapsed := 0.01

		for _, renderObj := range w.renderObjs {
			renderObj.Update(elapsed)
			renderObj.Render(projection, model, view, &w.Camera.Position, w.Light)
		}

		// 字体
		w.Text.Render(0, 50, mgl32.Vec4{1.0, 1.0, 1.0, 1.0})

		// Maintenance
		w.platform.PostRender()

	}
}

func (w *World) DrawAxis() {
	// logger.Info("DrawAxis...")
}

func (w *World) DrawLight() {
	// RenderObj
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

func (w *World) AddRenderObj(renderObj model.RenderObj) {
	w.renderObjs = append(w.renderObjs, renderObj)
}
