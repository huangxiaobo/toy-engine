package config

import "github.com/go-gl/mathgl/mgl32"

var Config = struct {
	WindowWidth  int32
	WindowHeight int32
	ClearColor   mgl32.Vec4
	ClipNear     float32
	ClipFar      float32
}{
	WindowWidth:  1200.0,
	WindowHeight: 800.0,
	ClearColor:   mgl32.Vec4{0.0, 0.0, 0.0, 0.0},
	ClipNear:     0.1,
	ClipFar:      500,
}
