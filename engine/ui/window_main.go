package ui

import (
	"fmt"
	"github.com/huangxiaobo/toy-engine/engine/utils"
	"github.com/inkyblackness/imgui-go/v4"
	"time"
)

const (
	millisPerSecond = 1000
	sleepDuration   = time.Millisecond * 25
)

const (
	ShowLightPanel int = 1 << 1
	ShowModelPanel int = 1 << 2
)

var ShowPanel int = 0

type WindowMain struct {
	noClose bool
	flags   WindowFlags

	menuShowGoDemoWindow bool
	menuScreenshot       bool

	World interface{}

	lightObjs   []interface{}
	lightWindow *WindowLight

	modelWindow *WindowModel
	modelItems  []ModelItem

	statusWindow *WindowStatus
}

func NewWindowMain(world interface{}) *WindowMain {
	wm := &WindowMain{
		flags:        WindowFlags{noResize: true, noMove: true, noMenu: false, noCollapse: true, noTitlebar: true},
		World:        world,
		modelItems:   make([]ModelItem, 0),
		lightWindow:  NewWindowLight(),
		modelWindow:  NewWindowModel(),
		statusWindow: NewWindowStatus(),
	}
	return wm
}

func (mw *WindowMain) Show(displaySize [2]float32) {
	imgui.SetNextWindowPosV(imgui.Vec2{X: 0, Y: 0}, imgui.ConditionNone, imgui.Vec2{})
	imgui.SetNextWindowSizeV(imgui.Vec2{X: 200, Y: displaySize[1]}, imgui.ConditionNone)

	if !imgui.BeginV("MainPanel", nil, mw.flags.combined()) {
		imgui.End()
		return
	}

	// MenuBar
	if imgui.BeginMenuBar() {
		if imgui.BeginMenu("Menu") {
			imgui.EndMenu()
		}
		if imgui.BeginMenu("Examples") {
			mw.menuShowGoDemoWindow = imgui.MenuItemV("Demo", "", mw.menuShowGoDemoWindow, true)
			mw.menuScreenshot = imgui.MenuItemV("Screenshot", "", mw.menuScreenshot, true)
			imgui.EndMenu()
		}

		imgui.EndMenuBar()
	}

	imgui.PushItemWidth(imgui.FontSize() * -12)

	if !imgui.CollapsingHeaderV("Scene", imgui.TreeNodeFlagsDefaultOpen) {
		imgui.End()
		return
	}

	// 显示light

	mw.addLightTreeNode()

	mw.addModelTreeNode()

	// 显示结束
	imgui.End()

	// 显示其他窗口
	if mw.menuShowGoDemoWindow {
		imgui.ShowDemoWindow(&mw.menuShowGoDemoWindow)
	}

	if ShowPanel == ShowLightPanel {
		mw.lightWindow.SetVisible(true)
		mw.lightWindow.Show(displaySize)
	} else {
		mw.lightWindow.SetVisible(false)
	}

	if ShowPanel == ShowModelPanel {
		mw.modelWindow.SetVisible(true)
		mw.modelWindow.Show(displaySize)
	} else {
		mw.modelWindow.SetVisible(false)
	}

	if mw.menuScreenshot {
		mw.ScreenCat(int(displaySize[0]), int(displaySize[1]))
		mw.menuScreenshot = false
	}
	mw.statusWindow.Show(displaySize)

}

func (mw *WindowMain) addLightTreeNode() {
	if imgui.TreeNodeV("light", imgui.TreeNodeFlagsDefaultOpen) {
		for i, lightObj := range mw.lightObjs {
			selected := false
			if mw.lightWindow.lightObj == lightObj && mw.lightWindow.visible == true {
				selected = true
			}

			label := fmt.Sprintf("light %d", i)
			if imgui.SelectableV(label, selected, imgui.SelectableFlagsAllowDoubleClick, imgui.Vec2{}) {
				mw.lightWindow.SetLight(lightObj)
				mw.lightWindow.SetVisible(false)
				ShowPanel = ShowLightPanel
			}
		}

		imgui.TreePop()
	}
}

type ModelItem struct {
	Name string
	Id   string
	Obj  interface{}
}

func (mw *WindowMain) addModelTreeNode() {
	selected := false
	selectedIdx := -1

	if imgui.TreeNodeV("model", imgui.TreeNodeFlagsDefaultOpen) {
		for i, item := range mw.modelItems {
			selected = false
			if mw.modelWindow.visible {
				if mw.modelWindow.modelObj == item.Obj {
					selected = true
				}
			}
			if imgui.SelectableV(item.Name, selected, imgui.SelectableFlagsAllowDoubleClick, imgui.Vec2{}) {
				selectedIdx = i
			}
		}
		imgui.TreePop()
	}

	if selectedIdx >= 0 {
		mw.modelWindow.SetRenderObj(mw.modelItems[selectedIdx].Obj)
		ShowPanel = ShowModelPanel
	}
}

func (mw *WindowMain) SetModelItem(items []ModelItem) {
	mw.modelItems = items
}

func (mw *WindowMain) AddLight(light interface{}) {
	mw.lightObjs = append(mw.lightObjs, light)
}

func (mw *WindowMain) AddModelItem(item ModelItem) {
	mw.modelItems = append(mw.modelItems, item)
}

func (mw *WindowMain) ScreenCat(width, height int) {
	utils.Screenshot(width, height)

}
