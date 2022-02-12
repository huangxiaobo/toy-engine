package model

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/light"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/huangxiaobo/toy-engine/engine/material"
	"github.com/huangxiaobo/toy-engine/engine/mesh"
	"github.com/huangxiaobo/toy-engine/engine/shader"
	"github.com/huangxiaobo/toy-engine/engine/technique"
	"github.com/huangxiaobo/toy-engine/engine/utils"
	"path/filepath"
)

type Ground struct {
	Meshes   []mesh.Mesh
	BasePath string
	FileName string

	Name string
	Id   string

	Material *material.Material
	effect   *technique.LightingTechnique
	shader   *shader.Shader

	Position mgl32.Vec3
	model    mgl32.Mat4

	DrawMode uint32
}

func NewGround(xmlModel config.XmlModel) (Ground, error) {
	basePath := filepath.Join(utils.GetCurrentDir(), "resource/model", xmlModel.Name)
	g := Ground{
		BasePath: basePath,
		Position: mgl32.Vec3{0, 0, 0},
		model:    mgl32.Ident4(),
		Name:     xmlModel.Name,
		Id:       xmlModel.Id,
		FileName: xmlModel.Mesh.File,
		effect:   &technique.LightingTechnique{},
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

	return g, nil
}

func (g *Ground) Init() {
	// mesh init
	g.Meshes = mesh.NewMeshGround()

	// shader
	if err := g.shader.Init(); err != nil {
		logger.Error(err)
		panic(err)
	}
	g.effect.Init(g.shader)
}

func (g *Ground) Dispose() {
	for i := 0; i < len(g.Meshes); i++ {
		g.Meshes[i].Dispose()
	}
}

func (g *Ground) SetPosition(p mgl32.Vec3) {
	g.Position = p
}

func (g *Ground) Update(elapsed float64) {
}

func (g *Ground) PreRender() {
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)
}

func (g *Ground) Render(projection, model, view mgl32.Mat4, eyePosition *mgl32.Vec3, lights []*light.PointLight) {
	// RenderObj
	model = model.Mul4(g.model)
	mvp := projection.Mul4(view).Mul4(model)

	// Effect
	g.effect.Enable()
	g.effect.SetProjectMatrix(&projection)
	g.effect.SetViewMatrix(&view)
	g.effect.SetModelMatrix(&model)
	g.effect.SetWVP(&mvp)
	g.effect.SetEyeWorldPos(eyePosition)

	gl.BindFragDataLocation(g.effect.ShaderObj.Program, 0, gl.Str("color\x00"))

	gl.LineWidth(30)
	for _, m := range g.Meshes {
		m.Draw(g.effect.ShaderObj.Program)
	}
	g.effect.Disable()
}

func (g *Ground) PostRender() {
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
}
