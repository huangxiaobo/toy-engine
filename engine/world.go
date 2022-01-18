package engine

import (
	"github.com/huangxiaobo/toy-engine/engine/logger"
	_ "image/png"
	"time"

	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/camera"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/light"
	"github.com/huangxiaobo/toy-engine/engine/model"
	"github.com/huangxiaobo/toy-engine/engine/text"
)

const (
	millisPerSecond = 1000
	sleepDuration   = time.Millisecond * 25
)

type World struct {
	Light      *light.PointLight
	renderObjs []model.RenderObj
	Camera     *camera.Camera
	Text       *text.Text
}

func (w *World) initGL() {
	// Initialize gl
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
	//gl.FrontFace(gl.CCW)
	gl.FrontFace(gl.CW)
}

func (w *World) Init() error {
	w.initGL()

	// 初始化摄像机
	w.Camera = new(camera.Camera)
	w.Camera.Init(mgl32.Vec3{0.0, 50.0, 50.0}, mgl32.Vec3{-0.0, -0.0, -0.0})

	// 初始化灯光
	w.Light = &light.PointLight{Position: mgl32.Vec4{0, 50.0, 0, 0.0}, Color: mgl32.Vec3{1.0, 1.0, 1.0}}
	w.Light.Init()

	// Text
	w.Text, _ = text.NewText("Toy引擎", 32)

	return nil
}

func (w *World) Destroy() {
}

func (w *World) Run() {

	gl.ClearColor(0.0, 0.0, 0.0, 1.0)
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

	projection := mgl32.Perspective(
		mgl32.DegToRad(w.Camera.Zoom),
		float32(config.Config.WindowWidth)/float32(config.Config.WindowHeight),
		0.1,
		100.0,
	)

	view := w.Camera.GetViewMatrix()
	model := mgl32.Ident4()
	//mvp := projection.Mul4(view).Mul4(model)

	//w.DrawAxis()
	w.DrawLight()
	// Update
	elapsed := 0.01

	for _, renderObj := range w.renderObjs {
		renderObj.Update(elapsed)
		renderObj.PreRender()
		renderObj.Render(projection, model, view, &w.Camera.Position, w.Light)
		renderObj.PostRender()
	}

	w.Text.Render(0, 50, mgl32.Vec4{1.0, 1.0, 1.0, 1.0})
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

func (w *World) GetRenderObj() []model.RenderObj {
	return w.renderObjs
}
