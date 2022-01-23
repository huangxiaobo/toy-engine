package ui

import (
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

	ModelWidget modelWidget
}

func NewWindowMain() *WindowMain {
	wm := &WindowMain{
		flags:       WindowFlags{noResize: true, noMove: true, noMenu: true},
		ModelWidget: modelWidget{selectItemIdx: -1},
	}
	return wm
}

func (mw *WindowMain) ShowWindowMain() {
	imgui.SetNextWindowPosV(imgui.Vec2{X: 0, Y: 20}, imgui.ConditionFirstUseEver, imgui.Vec2{})
	imgui.SetNextWindowSizeV(imgui.Vec2{X: 350, Y: 350}, imgui.ConditionFirstUseEver)

	if !imgui.BeginV("MainPanel", nil, mw.flags.combined()) {
		// Early out if the window is collapsed, as an optimization.
		imgui.End()
		return
	}

	// We choose a width proportional to our font size.
	imgui.PushItemWidth(imgui.FontSize() * -12)

	// All demo contents
	mw.ModelWidget.show()

	// End of ShowDemoWindow()
	imgui.End()
}

type ModelItem struct {
	Name string
	Id   string
}

type ModelItemSlectFunc func(item ModelItem)

type modelWidget struct {
	background     bool
	borders        bool
	noInnerBorders bool
	header         bool

	selectItemIdx  int
	modelItems     []ModelItem
	selectItemFunc ModelItemSlectFunc
}

func (mw *modelWidget) show() {
	if !imgui.CollapsingHeaderV("World", 1<<5) {
		return
	}

	if imgui.TreeNodeV("model", 1<<5) {

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

func (mw *WindowMain) NotifyModelItemChange(function ModelItemSlectFunc) {
	mw.ModelWidget.selectItemFunc = function
}
