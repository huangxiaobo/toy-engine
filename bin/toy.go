package main

import (
	_ "image/png"
	"log"
	"runtime"
	"toy/pkg/engine"
	"toy/pkg/engine/mesh"
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

	world.AddRenderObj(&mesh.Cube{
		Name:        "cube",
		ObjFilePath: "./resource/cube.obj",
		VsFilePath:  "./resource/cube.vs",
		FsFilePath:  "./resource/cube.fs",
	})

	world.Run()
}
