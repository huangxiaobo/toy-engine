package main

import (
	"github.com/huangxiaobo/toy-engine/engine"
)

func main() {

	world := engine.NewWorld("./resource/world.xml")
	defer world.Destroy()

	world.Run()
}
