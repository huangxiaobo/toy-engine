package engine

import (
	"fmt"
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/model"
	"github.com/huangxiaobo/toy-engine/engine/platforms"
	"github.com/huangxiaobo/toy-engine/engine/text"
	"github.com/huangxiaobo/toy-engine/engine/ui"
	"github.com/huangxiaobo/toy-engine/engine/utils"
	"github.com/inkyblackness/imgui-go/v4"
	_ "image/png"
	"log"
	"os"
	"reflect"
	"time"

	"github.com/huangxiaobo/toy-engine/engine/camera"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/light"
	"github.com/huangxiaobo/toy-engine/engine/logger"
)

const (
	millisPerSecond = 1000
	sleepDuration   = time.Millisecond * 25
)

type World struct {
	context  *imgui.Context
	platform *platforms.SDL
	imguiIO  imgui.IO
	renderer *platforms.OpenGL4

	xmlWorld   *config.XmlWorld
	Lights     []*light.PointLight
	renderObjs []model.RenderObj
	Camera     *camera.Camera
	Text       *text.Text

	// 界面
	uiWindowMain *ui.WindowMain
	bRun         bool
}

func NewWorld(configFile string) *World {
	world := &World{}
	err := world.Init(configFile)
	if err != nil {
		log.Fatalln("failed to initialize world:", err)
	}
	return world
}

func (w *World) initSDL() {
	var err error

	windowWidth := config.Config.WindowWidth
	windowHeight := config.Config.WindowHeight

	w.platform, err = platforms.NewSDL(w.imguiIO, platforms.SDLClientAPIOpenGL4, windowWidth, windowHeight)
	if err != nil {
		panic(err)
	}

	w.renderer, err = platforms.NewOpenGL4(w.imguiIO)
	if err != nil {
		_, _ = fmt.Fprintf(os.Stderr, "%v\n", err)
		os.Exit(-1)
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
	gl.FrontFace(gl.CCW) // 默认是 GL_CCW : Counter Clock Wind 逆时针方向
}

func (w *World) initModels() {

	for _, xmlMode := range w.xmlWorld.XMLModels.XMLModels {
		resourceClass := xmlMode.XmlResourceClass

		switch resourceClass {
		case "Ground":
			obj, _ := model.NewGround(xmlMode)
			w.renderObjs = append(w.renderObjs, &obj)
		case "Model":
			obj, _ := model.NewModel(xmlMode)
			w.renderObjs = append(w.renderObjs, &obj)

		}
	}
}

func (w *World) initUI() {
	imgui.PushStyleVarFloat(imgui.StyleVarWindowBorderSize, 1)
	imgui.PushStyleVarFloat(imgui.StyleVarWindowRounding, 6)
	imgui.PushStyleVarFloat(imgui.StyleVarFrameRounding, 6)
	imgui.PushStyleVarFloat(imgui.StyleVarFrameBorderSize, 1)

	w.uiWindowMain = ui.NewWindowMain(w)

	for _, l := range w.Lights {
		w.uiWindowMain.AddLight(l)
	}

	for _, renderObj := range w.renderObjs {
		name := reflect.ValueOf(renderObj).Elem().FieldByName("Name").String()
		id := reflect.ValueOf(renderObj).Elem().FieldByName("Id").String()

		w.uiWindowMain.AddModelItem(ui.ModelItem{Name: name, Id: id, Obj: renderObj})
	}
}

func (w *World) Init(configFile string) error {
	w.xmlWorld = config.InitXML(configFile)
	w.context = imgui.CreateContext(nil)

	w.imguiIO = imgui.CurrentIO()

	w.initSDL()
	//w.initGL()
	w.initModels()

	// 初始化摄像机
	xmlCamera := w.xmlWorld.XMLCamera
	w.Camera = new(camera.Camera)
	w.Camera.Init(xmlCamera.XMLPosition.XYZ(), xmlCamera.XMLTarget.XYZ())

	// 初始化灯光

	xmlLights := w.xmlWorld.XMLLights.XMLLights
	for _, xmlLight := range xmlLights {
		w.Lights = append(w.Lights, light.NewPointLight(xmlLight))
	}

	// Text
	w.Text = text.NewText("Toy引擎", 32, mgl32.Vec3{1, 0, 0})

	w.initUI()

	w.bRun = true
	return nil
}

func (w *World) Destroy() {
	w.renderer.Dispose()
	w.context.Destroy()
	w.platform.Dispose()

}

// Platform covers mouse/keyboard/gamepad inputs, cursor shape, timing, windowing.
type Platform interface {
	// ShouldStop is regularly called as the abort condition for the program loop.
	ShouldStop() bool
	// ProcessEvents is called once per render loop to dispatch any pending events.
	ProcessEvents()
	// DisplaySize returns the dimension of the display.
	DisplaySize() [2]float32
	// FramebufferSize returns the dimension of the framebuffer.
	FramebufferSize() [2]float32
	// NewFrame marks the begin of a render pass. It must update the imgui IO state according to user input (mouse, keyboard, ...)
	NewFrame()
	// PostRender marks the completion of one render pass. Typically this causes the display buffer to be swapped.
	PostRender()
	// ClipboardText returns the current text of the clipboard, if available.
	ClipboardText() (string, error)
	// SetClipboardText sets the text as the current text of the clipboard.
	SetClipboardText(text string)
}

type clipboard struct {
	platform Platform
}

func (board clipboard) Text() (string, error) {
	return board.platform.ClipboardText()
}

func (board clipboard) SetText(text string) {
	board.platform.SetClipboardText(text)
}

var cnt = 0

func (w *World) Run() {
	imgui.CurrentIO().SetClipboard(clipboard{platform: w.platform})

	for !w.platform.ShouldStop() {
		w.platform.ProcessEvents()

		// Signal start of a new frame
		w.platform.NewFrame()
		imgui.NewFrame()

		displaySize := w.platform.DisplaySize()
		w.uiWindowMain.Show(displaySize)

		// Rendering
		imgui.Render() // This call only creates the draw data list. Actual rendering to framebuffer is done below.

		w.renderer.PreRender(config.Config.ClearColor.Vec3())

		projection := mgl32.Perspective(
			mgl32.DegToRad(w.Camera.Zoom),
			float32(config.Config.WindowHeight/config.Config.WindowHeight),
			config.Config.ClipNear,
			config.Config.ClipFar,
		)
		view := w.Camera.GetViewMatrix()
		model := mgl32.Ident4()
		//mvp := projection.Mul4(view).Mul4(model)

		// Update
		elapsed := 0.01

		//w.DrawAxis()
		w.DrawLight(elapsed)

		for _, renderObj := range w.renderObjs {
			renderObj.Update(elapsed)
			renderObj.PreRender()
			renderObj.Render(projection, model, view, &w.Camera.Position, w.Lights)
			renderObj.PostRender()
		}

		// Logo
		w.Text.Render(int(displaySize[0]/2-50), 0)

		// Maintenance
		w.renderer.Render(w.platform.DisplaySize(), w.platform.FramebufferSize(), imgui.RenderedDrawData())
		w.platform.PostRender()

		if cnt > 0 && cnt%1000 == 0 {
			utils.Screenshot(int(displaySize[0]), int(displaySize[1]))
		}
		cnt += 1

		// sleep to avoid 100% CPU usage for this demo
		<-time.After(sleepDuration)
	}
}

func (w *World) DrawLight(elapsed float64) {
	// RenderObj
	width := float32(config.Config.WindowWidth)
	height := float32(config.Config.WindowHeight)
	projection := mgl32.Perspective(
		mgl32.DegToRad(w.Camera.Zoom),
		width/height,
		config.Config.ClipNear,
		config.Config.ClipFar,
	)
	view := w.Camera.GetViewMatrix()
	model := mgl32.Ident4()

	for _, l := range w.Lights {
		l.Update(elapsed)
		l.Render(projection, view, model)
	}
}
