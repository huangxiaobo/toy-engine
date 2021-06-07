package camera

import (
	"math"

	"github.com/go-gl/mathgl/mgl32"
)

var CameraMovement int32

const (
	FORWARD  int = 0
	BACKWARD int = 1
	LEFT         = 2
	RIGHT        = 3
)

// Default camera values
const (
	YAW         float32 = -90.0
	PITCH       float32 = 0.0
	SPEED               = 2.5
	SENSITIVITY         = 0.1
	ZOOM                = 45.0
)

type Camera struct {
	// camera attributes
	Position mgl32.Vec3
	Front    mgl32.Vec3
	Right    mgl32.Vec3
	Up       mgl32.Vec3
	WorldUp  mgl32.Vec3

	// euler angles
	Yaw   float32
	Pitch float32

	// camera options
	MovementSpeed    float32
	MouseSensitivity float32
	Zoom             float32
}

func (c *Camera) Init(position mgl32.Vec3, front mgl32.Vec3) {
	c.Position = position
	c.Front = front
	c.Up = mgl32.Vec3{0.0, 1.0, 0.0}
	c.WorldUp = mgl32.Vec3{0.0, 1.0, 0.0}
	c.Yaw = YAW
	c.Pitch = PITCH
	c.Zoom = ZOOM

}

func (c *Camera) GetViewMatrix() mgl32.Mat4 {
	return mgl32.LookAtV(c.Position, c.Position.Add(c.Front), c.Up)
}

func (c *Camera) ProcessKeyboard(direction int, deltaTime float32) {
	velocity := c.MovementSpeed * deltaTime
	switch direction {
	case FORWARD:
		c.Position = c.Position.Add(c.Front.Mul(velocity))
	case BACKWARD:
		c.Position = c.Position.Sub(c.Front.Mul(velocity))
	case LEFT:
		c.Position = c.Position.Sub(c.Right.Mul(velocity))
	case RIGHT:
		c.Position = c.Position.Add(c.Right.Mul(velocity))
	}
}

func (c *Camera) ProcessMouseMovement(xOffset float32, yOffset float32, constrainPitch bool) {
	xOffset = xOffset * c.MouseSensitivity
	yOffset = yOffset * c.MouseSensitivity

	c.Yaw += xOffset
	c.Pitch += yOffset

	if constrainPitch {
		if c.Pitch > 89.0 {
			c.Pitch = 89.0
		}
		if c.Pitch < -89.0 {
			c.Pitch = -89.0
		}
	}

	c.updateCameraVectors()
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

func (c *Camera) updateCameraVectors() {
	var front mgl32.Vec3
	front[0] = float32(math.Cos(float64(mgl32.DegToRad(c.Yaw))) * math.Cos(float64(mgl32.DegToRad(c.Pitch))))
	front[1] = float32(math.Sin(float64(mgl32.DegToRad(c.Pitch))))
	front[2] = float32(math.Sin(float64(mgl32.DegToRad(c.Yaw))) * math.Cos(float64(mgl32.DegToRad(c.Pitch))))

	c.Front = front.Normalize()
	c.Right = c.Front.Cross(c.WorldUp).Normalize()
	c.Up = c.Right.Cross(c.Front).Normalize()

}
