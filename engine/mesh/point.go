package mesh

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
)

func NewMeshPoint() []Mesh {

	meshes := GenPointMesh()

	for i := 0; i < len(meshes); i++ {
		meshes[i].Setup()
	}

	return meshes
}

func GenPointMesh() []Mesh {
	meshes := make([]Mesh, 0)

	m := Mesh{
		DrawMode: gl.POINTS,
	}

	v := Vertex{
		Position:  mgl32.Vec3{0, 0, 0},
		Color:     mgl32.Vec3{0, 0, 0},
		Normal:    mgl32.Vec3{0.0, 1.0, 0.0},
		TexCoords: mgl32.Vec2{0.0, 0.0},
		Tangent:   mgl32.Vec3{0.0, 0.0, 0.0},
		Bitangent: mgl32.Vec3{0.0, 0.0, 0.0},
	}

	m.Vertices = append(m.Vertices, v)
	m.Indices = append(m.Indices, 0)

	meshes = append(meshes, m)

	return meshes
}
