package ui

import (
	"fmt"
	"github.com/inkyblackness/imgui-go/v4"
)

type WindowStatus struct {
	noClose bool
	visible bool
	flags   WindowFlags
	size    imgui.Vec2
	height  int32
}

func NewWindowStatus() *WindowStatus {
	return &WindowStatus{
		visible: true,
		flags:   WindowFlags{noTitlebar: true, noResize: true, noMenu: true, noCollapse: true, noBackground: true},
		size:    imgui.Vec2{X: 600, Y: 30},
		height:  20,
	}

}

func (w *WindowStatus) adjustPost() {

}

func (w *WindowStatus) Show(displaySize [2]float32) {
	pos := imgui.Vec2{X: displaySize[0]/2 - w.size.X/2, Y: 0}
	imgui.SetNextWindowPosV(pos, imgui.ConditionNone, imgui.Vec2{})
	imgui.SetNextWindowSizeV(w.size, imgui.ConditionNone)

	if !imgui.BeginV("WindowStatus", &w.visible, w.flags.combined()) {
		// Early out if the window is collapsed, as an optimization.
		imgui.End()
		return
	}

	// We choose a width proportional to our font size.
	imgui.PushItemWidth(imgui.FontSize() * -36)

	// Calculate FPS
	text := fmt.Sprintf("Application average %.3f ms/frame (%.1f FPS)",
		millisPerSecond/imgui.CurrentIO().Framerate(),
		imgui.CurrentIO().Framerate(),
	)

	windowWidth := imgui.WindowWidth()
	textWidth := imgui.CalcTextSize(text, false, imgui.FontSize()*-12).X
	x := windowWidth/2 - textWidth/2
	imgui.SetCursorPos(imgui.Vec2{X: x})
	imgui.Text(text)

	// End of ShowDemoWindow()
	imgui.End()

}
