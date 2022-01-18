package window

import (
	"fmt"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/model"
	"github.com/therecipe/qt/core"
	"github.com/therecipe/qt/gui"
	"github.com/therecipe/qt/widgets"
	"log"
	"time"
)

type TreeModelItem struct {
	Name string
	Id   string
}

func SetupTree(window *MainWindow, treeItems []TreeModelItem) {

	treeWidget := window.TreeWidget

	treeWidget.HeaderItem().SetHidden(true)
	treeWidget.HeaderItem().SetDisabled(true)
	treeWidget.HeaderItem().SetText(0, "世界")

	modelTreeNode := widgets.NewQTreeWidgetItem(0)
	modelTreeNode.SetText(0, "模型")
	treeWidget.AddTopLevelItem(modelTreeNode)

	var modelTreeChildren []*widgets.QTreeWidgetItem

	for _, treeItem := range treeItems {
		item := widgets.NewQTreeWidgetItem(1)
		item.SetText(0, fmt.Sprintf("%s.%s", treeItem.Name, treeItem.Id))

		modelTreeChildren = append(modelTreeChildren, item)
	}
	modelTreeNode.AddChildren(modelTreeChildren)
	modelTreeNode.SetExpanded(true)

	treeWidget.ConnectKeyPressEvent(func(event *gui.QKeyEvent) {
		fmt.Printf("treeWidget.ConnectKeyPressEvent:%+v %d\n", event, event.Key())
	})
}

func SetupOpenGLWidget(window *MainWindow, world *engine.World) *MainWindow {

	openglWidget := window.OpenGLWidget
	openglWidget.ConnectInitializeGL(func() {
		err := world.Init()
		if err != nil {
			log.Fatalln("failed to initialize world:", err)
		}
		ground, _ := model.NewGround("./assets/model/ground/ground.xml")
		ground.SetScale(mgl32.Vec3{1, 1, 1})
		//obj.DrawMode = model.DRAW_MODEL_LINES
		world.AddRenderObj(&ground)
		obj, _ := model.NewModel("./assets/model/bunny/bunny.xml")
		world.AddRenderObj(&obj)

		treeItems := make([]TreeModelItem, 0)
		treeItems = append(treeItems, TreeModelItem{Name: ground.Name, Id: ground.Id})
		treeItems = append(treeItems, TreeModelItem{Name: obj.Name, Id: obj.Id})

		SetupTree(window, treeItems)
	})

	openglWidget.ConnectResizeGL(func(w int, h int) {
		config.Config.WindowWidth = int32(w)
		config.Config.WindowHeight = int32(h)
		openglWidget.Update()
	})

	openglWidget.ConnectUpdate(func() {
		world.Run()
	})

	openglWidget.ConnectPaintGL(func() {
		world.Run()
	})
	openglWidget.ConnectMousePressEvent(func(event *gui.QMouseEvent) {
		openglWidget.SetFocus(core.Qt__MouseFocusReason)
		fmt.Printf("openglWidget.ConnectMousePressEvent:%+v\n", event)
	})
	openglWidget.ConnectMouseReleaseEvent(func(event *gui.QMouseEvent) {
		fmt.Printf("openglWidget.ConnectMouseReleaseEvent:%+v\n", event)
	})

	openglWidget.ConnectKeyPressEvent(func(event *gui.QKeyEvent) {
		fmt.Printf("openglWidget.ConnectKeyPressEvent:%+v, %d\n", event, event.Key())
		fmt.Printf("openglWidget.ConnectKeyPressEvent:%d, %d %d\n", core.Qt__Key_F1, event.Key(), core.Qt__Key(event.Key()))

		switch core.Qt__Key(event.Key()) {
		case core.Qt__Key_F1:
			fmt.Printf("up")
		}
	})

	openglWidget.ConnectKeyReleaseEvent(func(event *gui.QKeyEvent) {
		fmt.Printf("openglWidget.ConnectKeyReleaseEvent%+v\n", event)
	})

	tk := time.NewTimer(time.Millisecond * 30)
	go func() {
		for {
			select {
			case <-tk.C:
				openglWidget.Update()
				tk.Reset(time.Millisecond * 30)
			}
		}
	}()

	window.ConnectMousePressEvent(func(event *gui.QMouseEvent) {
		fmt.Printf("window.ConnectMousePressEvent%+v\n", event)
	})
	window.ConnectKeyPressEvent(func(event *gui.QKeyEvent) {
		fmt.Printf("window.ConnectKeyPressEvent%+v\n", event)
	})
	return window
}

func SetupUi(world *engine.World) {
	window := NewMainWindow(nil)
	window.SetWindowTitle("ToyEngine")
	SetupOpenGLWidget(window, world)
	window.Show()
}
