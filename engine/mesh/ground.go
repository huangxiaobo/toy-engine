package mesh

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
)

func NewMeshGround() []Mesh {

	meshes := GenGroundMesh()

	for i := 0; i < len(meshes); i++ {
		meshes[i].Setup()
	}

	return meshes
}

func GenGroundMesh() []Mesh {
	meshes := make([]Mesh, 0)

	m := Mesh{
		DrawMode: gl.LINES,
	}

	var gridNum int32 = 10
	var gridStrip float32 = 5
	var gridWidth = float32(gridNum) * gridStrip / 2

	// draw grid
	var i uint32 = 0
	for k := -gridNum / 2; k <= gridNum/2; k += 1 {
		if k == 0 {
			continue
		}
		for _, item := range [][]float32{
			{float32(k) * gridStrip, +gridWidth},
			{float32(k) * gridStrip, -gridWidth},
			{+gridWidth, float32(k) * gridStrip},
			{-gridWidth, float32(k) * gridStrip},
		} {
			x := item[0]
			y := float32(0.0)
			z := item[1]

			v1 := Vertex{
				Position:  mgl32.Vec3{x, y, z},
				Normal:    mgl32.Vec3{0.0, 1.0, 0.0},
				TexCoords: mgl32.Vec2{0.0, 0.0},
				Tangent:   mgl32.Vec3{0.0, 0.0, 0.0},
				Bitangent: mgl32.Vec3{0.0, 0.0, 0.0},
			}

			m.Vertices = append(m.Vertices, v1)
			m.Indices = append(m.Indices, i)
			i += 1
		}

	}

	meshes = append(meshes, m)

	m = Mesh{
		DrawMode: gl.LINES,
	}
	// draw axis
	for i, item := range [][]float32{
		{0, 0.0, +gridWidth},
		{0, 0.0, -gridWidth},
		{+gridWidth, 0.0, 0},
		{-gridWidth, 0.0, 0},
		{0.0, +gridWidth, 0.0},
		{0.0, -1, 0.0},
	} {
		x := item[0]
		y := item[1]
		z := item[2]

		v1 := Vertex{
			Position:  mgl32.Vec3{x, y, z},
			Color:     mgl32.Vec3{mgl32.Abs(x), mgl32.Abs(y), mgl32.Abs(z)},
			Normal:    mgl32.Vec3{0.0, 1.0, 0.0},
			TexCoords: mgl32.Vec2{0.0, 0.0},
			Tangent:   mgl32.Vec3{0.0, 0.0, 0.0},
			Bitangent: mgl32.Vec3{0.0, 0.0, 0.0},
		}

		m.Vertices = append(m.Vertices, v1)
		m.Indices = append(m.Indices, uint32(i))
	}

	meshes = append(meshes, m)
	return meshes
}
