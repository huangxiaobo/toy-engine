package ui

import (
	"fmt"
	"github.com/inkyblackness/imgui-go/v4"
)

type WindowMaterial struct {
	noClose bool
	visible bool
	flags   WindowFlags

	datsList []string
	dataDict map[string]interface{}
	content  string

	showDemoWindow bool
}

func NewWindowMaterial() *WindowMaterial {
	w := &WindowMaterial{
		visible:  false,
		flags:    WindowFlags{noResize: true, noMenu: true, noCollapse: true, noBackground: false},
		datsList: make([]string, 0),
		dataDict: make(map[string]interface{}, 0),
	}
	w.visible = false
	w.content = ""
	w.dataDict = make(map[string]interface{}, 0)
	w.datsList = make([]string, 0)

	return w
}

func (w *WindowMaterial) Reset() {
	w.content = ""
	w.dataDict = make(map[string]interface{}, 0)
	w.datsList = make([]string, 0)
}

func (w *WindowMaterial) Show(displaySize [2]float32) {
	if !w.visible {
		return
	}
	imgui.SetNextWindowPosV(imgui.Vec2{X: displaySize[0] - 350, Y: 20}, imgui.ConditionNone, imgui.Vec2{})
	imgui.SetNextWindowSizeV(imgui.Vec2{X: 350, Y: 680}, imgui.ConditionNone)

	if !imgui.BeginV("MaterialPanel", &w.visible, w.flags.combined()) {
		// Early out if the window is collapsed, as an optimization.
		imgui.End()
		return
	}

	// We choose a width proportional to our font size.
	imgui.PushItemWidth(imgui.FontSize() * -12)
	// imgui.Spacing()
	// All demo contents

	imgui.Checkbox("Demo Window", &w.showDemoWindow)

	if imgui.BeginTable("tableRowsAndColumns", 2) {

		//row := 0
		for _, key := range w.datsList {
			imgui.TableNextRow()

			value := w.dataDict[key]

			imgui.TableSetColumnIndex(0)
			imgui.Text(fmt.Sprintf("%s", key))
			imgui.TableSetColumnIndex(1)
			imgui.Text(fmt.Sprintf("%v", value))

		}

		imgui.EndTable()
	}

	//imgui.Text(WindowMaterial.content)

	// End of ShowDemoWindow()
	imgui.End()

	if w.showDemoWindow {
		// Normally user code doesn't need/want to call this because positions are saved in .ini file anyway.
		// Here we just want to make the demo initial state a bit more friendly!
		const demoX = 650
		const demoY = 20
		imgui.SetNextWindowPosV(imgui.Vec2{X: demoX, Y: demoY}, imgui.ConditionFirstUseEver, imgui.Vec2{})

		imgui.ShowDemoWindow(&w.showDemoWindow)
	}
}

func (w *WindowMaterial) AddMaterialAttr(attrName string, attrValue interface{}) {
	w.datsList = append(w.datsList, attrName)
	w.dataDict[attrName] = attrValue
	w.content += fmt.Sprintf(" %s, attr value: %v\n", attrName, attrValue)
}

func (w *WindowMaterial) SetVisible(visible bool) {
	w.visible = visible
}
