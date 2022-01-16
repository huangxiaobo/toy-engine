package main

import (
	"github.com/huangxiaobo/toy-engine/engine"
	"github.com/huangxiaobo/toy-engine/engine/window"
	"github.com/therecipe/qt/widgets"
	_ "image/png"
	"os"
	"runtime"
)

func init() {
	// GLFW event handling must run on the main OS thread
	runtime.LockOSThread()
}

func main() {

	world := new(engine.World)

	defer world.Destroy()

	widgets.NewQApplication(len(os.Args), os.Args)

	window.SetupUi(world)

	widgets.QApplication_Exec()

}
