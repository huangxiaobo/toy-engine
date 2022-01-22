package main

import (
	_ "image/png"
	"log"
	"runtime"

	"github.com/huangxiaobo/toy-engine/engine"
)

func init() {
	// GLFW event handling must run on the main OS thread
	runtime.LockOSThread()
}

func main() {

	world := new(engine.World)
	err := world.Init("./resource/world.xml")
	if err != nil {
		log.Fatalln("failed to initialize world:", err)
	}
	defer world.Destroy()

	world.Run()
}
