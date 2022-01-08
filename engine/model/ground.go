package model

import (
	"fmt"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/huangxiaobo/toy-engine/engine/material"
	"github.com/huangxiaobo/toy-engine/engine/shader"
	"github.com/huangxiaobo/toy-engine/engine/technique"
	"github.com/huangxiaobo/toy-engine/engine/texture"
	"strings"
)

type Ground struct {
	Model
}

func NewGround(b, f string, g bool, vertFile, fragFile string) (Model, error) {
	t := strings.Split(f, ".")
	gf := t[0] + ".gob"
	m := Model{
		BasePath:        b,
		FileName:        f,
		GobName:         gf,
		GammaCorrection: g,
	}
	m.texturesLoaded = make(map[string]texture.Texture)
	m.Meshes = GenGroundMesh()
	for i := 0; i < len(m.Meshes); i++ {
		for _, v := range m.Meshes[i].Vertices {
			fmt.Printf("%+v\n", v)
		}
		m.Meshes[i].setup()
	}

	m.effect = &technique.LightingTechnique{}

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
	m.effect.Init(s)

	//vertAttrib := uint32(gl.GetAttribLocation(s.Program, gl.Str("position\x00")))
	//normalAttrib := uint32(gl.GetAttribLocation(s.Program, gl.Str("normal\x00")))

	return m, nil
}

func GenGroundMesh() []Mesh {
	mesh := Mesh{}

	var x_num = 1
	var x_strip float32 = 10
	var z_num = 1
	var z_strip float32 = 10

	for zi := -z_num; zi <= z_num; zi += 1 {
		for xi := -x_num; xi <= x_num; xi += 1 {
			x := float32(xi) * x_strip
			y := float32(0.0)
			z := float32(zi) * z_strip

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

	x_row_num := uint32(2*x_num + 1)
	z_row_num := uint32(2*z_num + 1)
	var zi, xi uint32
	for zi = 0; zi < z_row_num-1; zi++ {
		for xi = 0; xi < x_row_num-1; xi++ {
			v0 := uint32(zi*x_row_num + xi)
			v1 := uint32(v0 + 1)
			v2 := uint32(v0 + x_row_num)
			v3 := uint32(v2 + 1)

			mesh.Indices = append(mesh.Indices, v0, v2, v1, v1, v2, v3)

		}
	}

	return []Mesh{mesh}
}
