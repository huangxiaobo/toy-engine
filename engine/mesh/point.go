package mesh

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
)

func NewMeshPoint(points ...mgl32.Vec3) []*Mesh {

	meshes := GenPointMesh(points...)

	for i := 0; i < len(meshes); i++ {
		meshes[i].Setup()
	}

	return meshes
}

func GenPointMesh(points ...mgl32.Vec3) []*Mesh {
	meshes := make([]*Mesh, 0)

	m := &Mesh{
		DrawMode: gl.POINTS,
	}

	for idx, point := range points {
		v := Vertex{
			Position:  mgl32.Vec3{point.X(), point.Y(), point.Z()},
			Color:     mgl32.Vec3{0, 0, 0},
			Normal:    mgl32.Vec3{0.0, 1.0, 0.0},
			TexCoords: mgl32.Vec2{0.0, 0.0},
			Tangent:   mgl32.Vec3{0.0, 0.0, 0.0},
			Bitangent: mgl32.Vec3{0.0, 0.0, 0.0},
		}

		m.Vertices = append(m.Vertices, v)
		m.Indices = append(m.Indices, uint32(idx))
	}
	meshes = append(meshes, m)

	return meshes
}
