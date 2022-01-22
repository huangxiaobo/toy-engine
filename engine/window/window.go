package window

import (
	"fmt"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/model"
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
		fmt.Println(event)
	})
}

func SetupOpenGLWidget(window *MainWindow, world *engine.World) *MainWindow {

	openglWidget := window.OpenGLWidget
	openglWidget.ConnectInitializeGL(func() {
		err := world.Init()
		if err != nil {
			log.Fatalln("failed to initialize world:", err)
		}
		ground, _ := model.NewGround("./resource/model/ground/ground.xml")
		ground.SetScale(mgl32.Vec3{1, 1, 1})
		//obj.DrawMode = model.DRAW_MODEL_LINES
		world.AddRenderObj(&ground)
		obj, _ := model.NewModel("./resource/model/bunny/bunny.xml")
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
	return window
}

func SetupUi(world *engine.World) {
	window := NewMainWindow(nil)
	SetupOpenGLWidget(window, world)
	window.Show()
}
