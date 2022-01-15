package window

import (
	"fmt"
	"github.com/inkyblackness/imgui-go/v4"
)

var WindowMaterial = struct {
	noClose bool
	visible bool
	flags   WindowFlags

	datsList []string
	dataDict map[string]interface{}
	content  string
}{
	visible:  false,
	flags:    WindowFlags{noResize: true},
	datsList: make([]string, 0),
	dataDict: make(map[string]interface{}, 0),
}

func InitWindowMaterial() {
	WindowMaterial.visible = true
	WindowMaterial.content = ""
	WindowMaterial.dataDict = make(map[string]interface{}, 0)
	WindowMaterial.datsList = make([]string, 0)

}

func ShowWindowMaterial() {
	if !WindowMaterial.visible {
		return
	}
	imgui.SetNextWindowPosV(imgui.Vec2{X: 0, Y: 20}, imgui.ConditionFirstUseEver, imgui.Vec2{})
	imgui.SetNextWindowSizeV(imgui.Vec2{X: 350, Y: 680}, imgui.ConditionFirstUseEver)

	if !imgui.BeginV("MaterialPanel", &WindowMaterial.visible, WindowMaterial.flags.combined()) {
		// Early out if the window is collapsed, as an optimization.
		imgui.End()
		return
	}

	// We choose a width proportional to our font size.
	imgui.PushItemWidth(imgui.FontSize() * -12)
	// imgui.Spacing()
	// All demo contents

	if imgui.BeginTable("tableRowsAndColumns", 2) {

		//row := 0
		for _, key := range WindowMaterial.datsList {
			imgui.TableNextRow()

			value := WindowMaterial.dataDict[key]

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
}

func AddMaterialAttr(attrName string, attrValue interface{}) {
	WindowMaterial.datsList = append(WindowMaterial.datsList, attrName)
	WindowMaterial.dataDict[attrName] = attrValue
	WindowMaterial.content += fmt.Sprintf(" %s, attr value: %v\n", attrName, attrValue)
}
