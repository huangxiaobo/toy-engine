package ui

import (
	"github.com/huangxiaobo/toy-engine/engine/utils"
	"github.com/inkyblackness/imgui-go/v4"
	"time"
)

const (
	millisPerSecond = 1000
	sleepDuration   = time.Millisecond * 25
)

type WindowMain struct {
	noClose bool
	flags   WindowFlags

	menuShowGoDemoWindow bool
	menuScreenCat        bool
	ModelWidget          modelWidget
}

func NewWindowMain() *WindowMain {
	wm := &WindowMain{
		flags:       WindowFlags{noResize: true, noMove: true, noMenu: false, noCollapse: true, noTitlebar: true},
		ModelWidget: modelWidget{selectItemIdx: -1},
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
			mw.menuShowGoDemoWindow = imgui.MenuItemV("Example", "", mw.menuShowGoDemoWindow, true)
			mw.menuScreenCat = imgui.MenuItemV("ScreenCat", "", mw.menuScreenCat, true)
			imgui.EndMenu()
		}

		imgui.EndMenuBar()
	}

	// We choose a width proportional to our font size.
	imgui.PushItemWidth(imgui.FontSize() * -12)

	// All demo contents
	mw.ModelWidget.show()

	if mw.menuShowGoDemoWindow {
		imgui.ShowDemoWindow(&mw.menuShowGoDemoWindow)
	}

	if mw.menuScreenCat {
		mw.ScreenCat(int(displaySize[0]), int(displaySize[1]))
		mw.menuScreenCat = false
	}

	// End of ShowDemoWindow()
	imgui.End()
}

type ModelItem struct {
	Name string
	Id   string
}

type ModelItemSelectFunc func(item ModelItem)

type modelWidget struct {
	background     bool
	borders        bool
	noInnerBorders bool
	header         bool

	selectItemIdx  int
	modelItems     []ModelItem
	selectItemFunc ModelItemSelectFunc
}

func (mw *modelWidget) show() {
	if !imgui.CollapsingHeaderV("World", imgui.TreeNodeFlagsDefaultOpen) {
		return
	}

	if imgui.TreeNodeV("model", imgui.TreeNodeFlagsDefaultOpen) {

		oldSelectItem := mw.selectItemIdx
		selected := false
		for i, item := range mw.modelItems {
			if mw.selectItemIdx == i+1 {
				selected = true
			} else {
				selected = false
			}
			if imgui.SelectableV(item.Name, selected, imgui.SelectableFlagsAllowDoubleClick, imgui.Vec2{}) {
				mw.selectItemIdx = i + 1
			}
		}
		if oldSelectItem != mw.selectItemIdx {
			mw.selectItemFunc(mw.modelItems[mw.selectItemIdx-1])
		}

		imgui.TreePop()
	}
}

func (mw *WindowMain) SetModelItem(items []ModelItem) {
	mw.ModelWidget.modelItems = items
}

func (mw *WindowMain) AddModelItem(item ModelItem) {
	mw.ModelWidget.modelItems = append(mw.ModelWidget.modelItems, item)
}

func (mw *WindowMain) NotifyModelItemChange(function ModelItemSelectFunc) {
	mw.ModelWidget.selectItemFunc = function
}

func (mw *WindowMain) ScreenCat(width, height int) {
	utils.Screenshot(width, height)

}
