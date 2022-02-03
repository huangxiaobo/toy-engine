package model

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/light"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/huangxiaobo/toy-engine/engine/material"
	"github.com/huangxiaobo/toy-engine/engine/shader"
	"github.com/huangxiaobo/toy-engine/engine/technique"
	"github.com/huangxiaobo/toy-engine/engine/texture"
	"github.com/huangxiaobo/toy-engine/engine/utils"
	"path/filepath"
	"sync"
)

type Ground struct {
	texturesLoaded  map[string]texture.Texture
	wg              sync.WaitGroup
	Meshes          []Mesh
	GammaCorrection bool
	BasePath        string
	FileName        string

	Name     string `ui:"Text"`
	Id       string `ui:"Text"`
	Material *material.Material
	effect   *technique.LightingTechnique
	shader   *shader.Shader

	model mgl32.Mat4

	DrawMode uint32 `ui:"DrawMode"`
}

func NewGround(xmlModel config.XmlModel) (Ground, error) {
	basePath := filepath.Join(utils.GetCurrentDir(), "resource/model", xmlModel.Name)
	g := Ground{
		BasePath:        basePath,
		model:           mgl32.Ident4(),
		Name:            xmlModel.Name,
		Id:              xmlModel.Id,
		FileName:        xmlModel.Mesh.File,
		GammaCorrection: xmlModel.GammaCorrection,
		texturesLoaded:  make(map[string]texture.Texture),
		effect:          &technique.LightingTechnique{},
		Material: &material.Material{
			AmbientColor:  xmlModel.Material.AmbientColor.RGB(),
			DiffuseColor:  xmlModel.Material.DiffuseColor.RGB(),
			SpecularColor: xmlModel.Material.SpecularColor.RGB(),
			Shininess:     xmlModel.Material.Shininess,
		},
		shader: &shader.Shader{
			VertFilePath: filepath.Join(basePath, xmlModel.Shader.VertFile),
			FragFilePath: filepath.Join(basePath, xmlModel.Shader.FragFile),
		},
	}

	g.Init()

	GenGroundMesh(&g)
	for i := 0; i < len(g.Meshes); i++ {
		g.Meshes[i].setup()
	}

	return g, nil
}

func GenGroundMesh(m *Ground) {
	mesh := Mesh{}

	var xNum = 100
	var xStrip float32 = 5
	var zNum = 100
	var zStrip float32 = 5

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

func (m *Ground) Init() {
	// shader
	if err := m.shader.Init(); err != nil {
		logger.Error(err)
		panic(err)
	}
	m.effect.Init(m.shader)

}

func (m *Ground) Dispose() {
	for i := 0; i < len(m.Meshes); i++ {
		gl.DeleteVertexArrays(1, &m.Meshes[i].vao)
		gl.DeleteBuffers(1, &m.Meshes[i].vbo)
		gl.DeleteBuffers(1, &m.Meshes[i].ebo)
	}
}
func (m *Ground) Update(elapsed float64) {
}

func (m *Ground) PreRender() {
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)
}

func (m *Ground) Render(projection, model, view mgl32.Mat4, eyePosition *mgl32.Vec3, lights []*light.PointLight) {
	// RenderObj
	model = model.Mul4(m.model)
	mvp := projection.Mul4(view).Mul4(model)

	// Effect
	m.effect.Enable()
	m.effect.SetProjectMatrix(&projection)
	m.effect.SetViewMatrix(&view)
	m.effect.SetModelMatrix(&model)
	m.effect.SetWVP(&mvp)
	m.effect.SetEyeWorldPos(eyePosition)

	gl.BindFragDataLocation(m.effect.ShaderObj.Program, 0, gl.Str("color\x00"))

	for _, mesh := range m.Meshes {
		mesh.draw(m.effect.ShaderObj.Program)
	}
	m.effect.Disable()
}

func (m *Ground) PostRender() {
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
}
