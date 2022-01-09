package model

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/huangxiaobo/toy-engine/engine/material"
	"github.com/huangxiaobo/toy-engine/engine/shader"
	"github.com/huangxiaobo/toy-engine/engine/technique"
	"github.com/huangxiaobo/toy-engine/engine/texture"
)

type Point struct {
	Model
}

func NewPoint(f string, g bool, vertFile, fragFile string) (Point, error) {
	m := Point{
		Model{
			FileName:        f,
			GammaCorrection: g,

			model: mgl32.Ident4(),
		},
	}
	m.texturesLoaded = make(map[string]texture.Texture)
	GenPointMesh(&m)

	for i := 0; i < len(m.Meshes); i++ {
		m.Meshes[i].setup()
	}

	// Material
	m.material = &material.Material{}
	m.material.AmbientColor = mgl32.Vec3{0.05, 0.1, 0.05}
	m.material.DiffuseColor = mgl32.Vec3{0.1, 0.2, 0.3}
	m.material.SpecularColor = mgl32.Vec3{0.0, 1.0, 0.0}
	m.material.Shininess = 2

	s := &shader.Shader{VertFilePath: vertFile, FragFilePath: fragFile}
	if err := s.Init(); err != nil {
		logger.Error(err)
		panic("errr")
	}

	m.effect = &technique.LightingTechnique{}
	m.effect.Init(s)
	return m, nil
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
