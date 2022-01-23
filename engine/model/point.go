package model

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/config"
)

type Point struct {
	*Model
}

func NewPoint(xmlModel config.XmlModel) (Point, error) {

	m, _ := NewModel(xmlModel)
	p := Point{
		Model: &m,
	}

	GenPointMesh(&p)
	for i := 0; i < len(m.Meshes); i++ {
		m.Meshes[i].setup()
	}

	return p, nil
}

func GenPointMesh(m *Point) {
	mesh := Mesh{}

	v0 := Vertex{
		Position:  mgl32.Vec3{10, 0, 0},
		Normal:    mgl32.Vec3{0.0, 1.0, 0.0},
		TexCoords: mgl32.Vec2{0.0, 0.0},
		Tangent:   mgl32.Vec3{0.0, 0.0, 0.0},
		Bitangent: mgl32.Vec3{0.0, 0.0, 0.0},
	}
	v1 := Vertex{
		Position:  mgl32.Vec3{0, 0, -10},
		Normal:    mgl32.Vec3{0.0, 1.0, 0.0},
		TexCoords: mgl32.Vec2{0.0, 0.0},
		Tangent:   mgl32.Vec3{0.0, 0.0, 0.0},
		Bitangent: mgl32.Vec3{0.0, 0.0, 0.0},
	}

	v2 := Vertex{
		Position:  mgl32.Vec3{10, 0, -10},
		Normal:    mgl32.Vec3{0.0, 1.0, 0.0},
		TexCoords: mgl32.Vec2{0.0, 0.0},
		Tangent:   mgl32.Vec3{0.0, 0.0, 0.0},
		Bitangent: mgl32.Vec3{0.0, 0.0, 0.0},
	}
	mesh.Vertices = append(mesh.Vertices, v0, v1, v2)
	mesh.Indices = append(mesh.Indices, 0, 1, 2)
	m.Meshes = append(m.Meshes, mesh)
}

func (m *Point) PreRender() {
	gl.PointSize(30)
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.POINT)
}

func (m *Point) PostRender() {
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
}
