package main

import (
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/model"
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
	err := world.Init()
	if err != nil {
		log.Fatalln("failed to initialize world:", err)
	}
	defer world.Destroy()

	ground, _ := model.NewGround(
		"ground",
		false,
		"./resource/shader/ground/shader.vert",
		"./resource/shader/ground/shader.frag",
	)
	ground.SetScale(mgl32.Vec3{1, 1, 1})
	//obj.DrawMode = model.DRAW_MODEL_LINES
	world.AddRenderObj(&ground)

	point, _ := model.NewPoint(
		"point",
		false,
		"./resource/shader/ground/shader.vert",
		"./resource/shader/ground/shader.frag",
	)
	point.SetScale(mgl32.Vec3{1, 1, 1})
	//obj.DrawMode = model.DRAW_MODEL_LINES
	world.AddRenderObj(&point)

	obj, _ := model.NewModel(
		"./resource/model/icosphere.obj",
		false,
		"./resource/shader/v2/shader.vert",
		"./resource/shader/v2/shader.frag",
	)
	obj.SetScale(mgl32.Vec3{10, 10, 10})
	world.AddRenderObj(&obj)

	world.Run()
}
