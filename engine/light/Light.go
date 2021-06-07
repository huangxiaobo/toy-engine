package light

import "github.com/go-gl/mathgl/mgl32"

type Light struct {
	Position  mgl32.Vec3
	Direction mgl32.Vec3
	Color     mgl32.Vec3
}
