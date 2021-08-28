package camera

import (
	"github.com/go-gl/mathgl/mgl32"
)

const (
	FORWARD  int = 0
	BACKWARD int = 1
	LEFT         = 2
	RIGHT        = 3
)

// Default camera values
const (
	YAW         float32 = -90.0
	PITCH       float32 = -45.0
	SPEED               = 0.025
	SENSITIVITY         = 0.1
	ZOOM                = 45.0
)

type Camera struct {
	// camera attributes
	Position mgl32.Vec3
	Target   mgl32.Vec3

	Front   mgl32.Vec3
	Right   mgl32.Vec3
	Up      mgl32.Vec3
	WorldUp mgl32.Vec3

	// camera options
	MovementSpeed    float32
	MouseSensitivity float32
	Zoom             float32
}

func (c *Camera) Init(position mgl32.Vec3, target mgl32.Vec3) {
	c.Position = position
	c.Target = target

	c.Front = c.Target.Sub(c.Position).Normalize()
	c.Up = mgl32.Vec3{0.0, 1.0, 0.0}
	c.WorldUp = mgl32.Vec3{0.0, 1.0, 0.0}
	c.MovementSpeed = SPEED

	c.Zoom = ZOOM
}

func (c *Camera) GetViewMatrix() mgl32.Mat4 {
	return mgl32.LookAtV(c.Position, c.Target, c.Up)
}

func (c *Camera) ProcessKeyboard(direction int, deltaTime float64) {
	c.Front = c.Target.Sub(c.Position).Normalize()

	velocity := c.MovementSpeed * float32(deltaTime)
	switch direction {
	case FORWARD:
		c.Position = mgl32.HomogRotate3DX(-velocity).Mul4x1(c.Position.Vec4(0)).Vec3()
	case BACKWARD:
		c.Position = mgl32.HomogRotate3DX(+velocity).Mul4x1(c.Position.Vec4(0)).Vec3()
	case LEFT:
		c.Position = mgl32.HomogRotate3DY(-0.1).Mul4x1(c.Position.Vec4(0)).Vec3()
	case RIGHT:
		c.Position = mgl32.HomogRotate3DY(+0.1).Mul4x1(c.Position.Vec4(0)).Vec3()
	}
}

func (c *Camera) ProcessMouseScroll(yOffset float32) {
	c.Zoom -= yOffset
	if c.Zoom < 1.0 {
		c.Zoom = 1.0
	}
	if c.Zoom > 45.0 {
		c.Zoom = 45.0
	}
}
