package main

import (
	_ "image/png"
	"log"
	"runtime"
	"toy/engine"
	"toy/engine/mesh"
)

func init() {
	// GLFW event handling must run on the main OS thread
	runtime.LockOSThread()
}

func main() {

	world := new(engine.World)
	err := world.Init()
	if err != nil {
		log.Fatalln("failed to initialize glfw:", err)
	}
	defer world.Destroy()

	world.AddRenderObj(&mesh.Axis{
		Name: "axis",
	})

	world.AddRenderObj(&mesh.Cube{
		Name: "cube",
	})

	world.Run()
}
