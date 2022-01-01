package platforms

import (
	"embed"
	"fmt"
	"github.com/veandco/go-sdl2/sdl"
	"github.com/veandco/go-sdl2/ttf"
	"runtime"
)

// SDLClientAPI identifies the render system that shall be initialized.
type SDLClientAPI string

// This is a list of SDLClientAPI constants.
const (
	SDLClientAPIOpenGL2 SDLClientAPI = "OpenGL2"
	SDLClientAPIOpenGL3 SDLClientAPI = "OpenGL3"
	SDLClientAPIOpenGL4 SDLClientAPI = "OpenGL4"
)

// SDL implements a platform based on github.com/veandco/go-sdl2 (v2).
type SDL struct {
	window     *sdl.Window
	shouldStop bool

	time        uint64
	buttonsDown [mouseButtonCount]bool
}

//go:embed *
var resourceData embed.FS

// NewSDL attempts to initialize an SDL context.
func NewSDL(clientAPI SDLClientAPI, windowWidth, windowHeight int32) (*SDL, error) {
	runtime.LockOSThread()

	err := sdl.Init(sdl.INIT_VIDEO)
	if err != nil {
		return nil, fmt.Errorf("failed to initialize SDL2: %w", err)
	}

	if err = ttf.Init(); err != nil {
		return nil, fmt.Errorf("filed to initialize ttf: %w", err)
	}

	window, err := sdl.CreateWindow("ImGui-Go SDL2+"+string(clientAPI)+" example",
		sdl.WINDOWPOS_CENTERED, sdl.WINDOWPOS_CENTERED, windowWidth, windowHeight, sdl.WINDOW_OPENGL)
	if err != nil {
		sdl.Quit()
		return nil, fmt.Errorf("failed to create window: %w", err)
	}

	platform := &SDL{
		window: window,
	}
	platform.setKeyMapping()

	switch clientAPI {
	case SDLClientAPIOpenGL2:
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_MAJOR_VERSION, 2)
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_MINOR_VERSION, 1)
	case SDLClientAPIOpenGL3:
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_MAJOR_VERSION, 3)
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_MINOR_VERSION, 2)
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_FLAGS, sdl.GL_CONTEXT_FORWARD_COMPATIBLE_FLAG)
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_PROFILE_MASK, sdl.GL_CONTEXT_PROFILE_CORE)
	case SDLClientAPIOpenGL4:
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_MAJOR_VERSION, 4)
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_MINOR_VERSION, 1)
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_FLAGS, sdl.GL_CONTEXT_FORWARD_COMPATIBLE_FLAG)
		_ = sdl.GLSetAttribute(sdl.GL_CONTEXT_PROFILE_MASK, sdl.GL_CONTEXT_PROFILE_CORE)
	default:
		platform.Dispose()
		return nil, ErrUnsupportedClientAPI
	}
	_ = sdl.GLSetAttribute(sdl.GL_DOUBLEBUFFER, 1)
	_ = sdl.GLSetAttribute(sdl.GL_DEPTH_SIZE, 24)
	_ = sdl.GLSetAttribute(sdl.GL_STENCIL_SIZE, 8)

	glContext, err := window.GLCreateContext()
	if err != nil {
		platform.Dispose()
		return nil, fmt.Errorf("failed to create OpenGL context: %w", err)
	}
	err = window.GLMakeCurrent(glContext)
	if err != nil {
		platform.Dispose()
		return nil, fmt.Errorf("failed to set current OpenGL context: %w", err)
	}

	_ = sdl.GLSetSwapInterval(1)

	return platform, nil
}

// Dispose cleans up the resources.
func (platform *SDL) Dispose() {
	if platform.window != nil {
		_ = platform.window.Destroy()
		platform.window = nil
	}
	sdl.Quit()
	ttf.Quit()
}

// ShouldStop returns true if the window is to be closed.
func (platform *SDL) ShouldStop() bool {
	return platform.shouldStop
}

// ProcessEvents handles all pending window events.
func (platform *SDL) ProcessEvents() {
	for event := sdl.PollEvent(); event != nil; event = sdl.PollEvent() {
		platform.processEvent(event)
	}
}

// DisplaySize returns the dimension of the display.
func (platform *SDL) DisplaySize() [2]float32 {
	w, h := platform.window.GetSize()
	return [2]float32{float32(w), float32(h)}
}

// FramebufferSize returns the dimension of the framebuffer.
func (platform *SDL) FramebufferSize() [2]float32 {
	w, h := platform.window.GLGetDrawableSize()
	return [2]float32{float32(w), float32(h)}
}

// NewFrame marks the begin of a render pass. It forwards all current state to imgui.CurrentIO().
func (platform *SDL) NewFrame() {
	// Setup display size (every frame to accommodate for window resizing)
	displaySize := platform.DisplaySize()

	// Setup time step (we don't use SDL_GetTicks() because it is using millisecond resolution)
	frequency := sdl.GetPerformanceFrequency()
	currentTime := sdl.GetPerformanceCounter()

	var deltaTime float32
	if platform.time > 0 {
		deltaTime = float32(currentTime-platform.time) / float32(frequency)
	} else {
		const fallbackDelta = 1.0 / 60.0
		deltaTime = fallbackDelta
	}

	fmt.Printf("displaySize: %v deltaTime: %f", displaySize, deltaTime)
	platform.time = currentTime

}

// PostRender performs a buffer swap.
func (platform *SDL) PostRender() {
	platform.window.GLSwap()
}

func (platform *SDL) setKeyMapping() {

}

func (platform *SDL) processEvent(event sdl.Event) {
	switch event.GetType() {
	case sdl.QUIT:
		platform.shouldStop = true
	case sdl.MOUSEWHEEL:
		wheelEvent := event.(*sdl.MouseWheelEvent)
		var deltaX, deltaY float32
		if wheelEvent.X > 0 {
			deltaX++
		} else if wheelEvent.X < 0 {
			deltaX--
		}
		if wheelEvent.Y > 0 {
			deltaY++
		} else if wheelEvent.Y < 0 {
			deltaY--
		}
	case sdl.MOUSEBUTTONDOWN:
		buttonEvent := event.(*sdl.MouseButtonEvent)
		switch buttonEvent.Button {
		case sdl.BUTTON_LEFT:
			platform.buttonsDown[mouseButtonPrimary] = true
		case sdl.BUTTON_RIGHT:
			platform.buttonsDown[mouseButtonSecondary] = true
		case sdl.BUTTON_MIDDLE:
			platform.buttonsDown[mouseButtonTertiary] = true
		}
	case sdl.TEXTINPUT:
		inputEvent := event.(*sdl.TextInputEvent)
		fmt.Println(inputEvent)
	case sdl.KEYDOWN:
		keyEvent := event.(*sdl.KeyboardEvent)
		fmt.Println(keyEvent)
	case sdl.KEYUP:
		keyEvent := event.(*sdl.KeyboardEvent)
		fmt.Println(keyEvent)
	}
}
