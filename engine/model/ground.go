package model

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/huangxiaobo/toy-engine/engine/material"
	"github.com/huangxiaobo/toy-engine/engine/shader"
	"github.com/huangxiaobo/toy-engine/engine/technique"
	"github.com/huangxiaobo/toy-engine/engine/texture"
	"os"
	"path/filepath"
)

type Ground struct {
	*Model
}

func NewGround(xmlModel config.XmlModel) (Ground, error) {
	cwd, _ := os.Getwd()
	m := Ground{
		Model: &Model{
			BasePath: filepath.Join(cwd, "resource/model", xmlModel.XMLAlias),
			model:    mgl32.Ident4(),
		},
	}

	m.Name = xmlModel.XMLAlias
	m.Id = xmlModel.XMLId
	m.FileName = xmlModel.XMLMesh.File
	m.GammaCorrection = xmlModel.GammaCorrection

	m.texturesLoaded = make(map[string]texture.Texture)
	GenGroundMesh(&m)
	for i := 0; i < len(m.Meshes); i++ {
		m.Meshes[i].setup()
	}

	m.effect = &technique.LightingTechnique{}

	// Material
	m.material = &material.Material{}
	m.material.AmbientColor = xmlModel.XMLMaterial.Ambientcolor.RGB()
	m.material.DiffuseColor = xmlModel.XMLMaterial.Diffusecolor.RGB()
	m.material.SpecularColor = xmlModel.XMLMaterial.Specularcolor.RGB()
	m.material.Shininess = xmlModel.XMLMaterial.Shininess

	s := &shader.Shader{
		VertFilePath: filepath.Join(m.BasePath, xmlModel.XmlShader.VertFile),
		FragFilePath: filepath.Join(m.BasePath, xmlModel.XmlShader.FragFile),
	}
	if err := s.Init(); err != nil {
		logger.Error(err)
		panic("errr")
	}
	m.effect.Init(s)

	return m, nil
}

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
