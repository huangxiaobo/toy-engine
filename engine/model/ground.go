package model

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/config"
)

type Ground struct {
	*Model
}

func NewGround(xmlModel config.XmlModel) (Ground, error) {

	m, _ := NewModel(xmlModel)
	g := Ground{
		Model: &m,
	}

	GenGroundMesh(&g)
	for i := 0; i < len(m.Meshes); i++ {
		m.Meshes[i].setup()
	}

	return g, nil
}

//
//func (g *Ground) Init() {
//	fmt.Printf("Ground.Init")
//	GenGroundMesh(g)
//	g.Model.Init()
//}
//
//// Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
//func (g *Ground) loadModel() error {
//	fmt.Printf("Ground.loadModel")
//	GenGroundMesh(g)
//	g.initGL()
//	return nil
//}

func GenGroundMesh(m *Ground) {
	mesh := Mesh{}

	var xNum = 100
	var xStrip float32 = 10
	var zNum = 100
	var zStrip float32 = 10

	for zi := -zNum; zi <= zNum; zi += 1 {
		for xi := -xNum; xi <= xNum; xi += 1 {
			x := float32(xi) * xStrip
			y := float32(0.0)
			z := float32(zi) * zStrip

			v := Vertex{
				Position:  mgl32.Vec3{x, y, z},
				Normal:    mgl32.Vec3{0.0, 1.0, 0.0},
				TexCoords: mgl32.Vec2{0.0, 0.0},
				Tangent:   mgl32.Vec3{0.0, 0.0, 0.0},
				Bitangent: mgl32.Vec3{0.0, 0.0, 0.0},
			}
			mesh.Vertices = append(mesh.Vertices, v)
		}
	}

	xRowNum := uint32(2*xNum + 1)
	zRowNum := uint32(2*zNum + 1)
	var zi, xi uint32
	for zi = 0; zi < zRowNum-1; zi++ {
		for xi = 0; xi < xRowNum-1; xi++ {
			v0 := uint32(zi*xRowNum + xi)
			v1 := uint32(v0 + 1)
			v2 := uint32(v0 + xRowNum)
			v3 := uint32(v2 + 1)

			mesh.Indices = append(mesh.Indices, v0, v2, v1, v1, v2, v3)

		}
	}

	m.Meshes = append(m.Meshes, mesh)
}

func (m *Ground) PreRender() {
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)
}

func (m *Ground) PostRender() {
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
}
