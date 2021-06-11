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
		Name:       "axis",
		VsFilePath: "./resource/axis/axis.vert",
		FsFilePath: "./resource/axis/axis.frag",
	})

	world.AddRenderObj(&mesh.WavefrontObject{
		Name:         "cube",
		ObjFilePath:  "./resource/cube/cube.obj",
		VertFilePath: "./resource/cube/cube.vert",
		FragFilePath: "./resource/cube/cube.frag",
	})

	world.Run()
}
