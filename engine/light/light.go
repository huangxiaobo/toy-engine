package light

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/huangxiaobo/toy-engine/engine/mesh"
	"github.com/huangxiaobo/toy-engine/engine/shader"
)

type Attenuation struct {
	Range    float32
	Constant float32
	Linear   float32
	Exp      float32
}

type PointLight struct {
	Color    mgl32.Vec3
	Position mgl32.Vec4

	AmbientIntensity float32
	DiffuseIntensity float32
	DiffuseColor     mgl32.Vec3
	SpecularColor    mgl32.Vec3
	Atten            *Attenuation

	shader            *shader.Shader
	projectionUniform int32
	viewUniform       int32
	modelUniform      int32
	colorUniform      int32

	model  mgl32.Mat4
	Meshes []*mesh.Mesh
}

type DirectionLight struct {
}

type Vertex struct {
	Position  mgl32.Vec3
	Normal    mgl32.Vec3
	TexCoords mgl32.Vec2
	Tangent   mgl32.Vec3
	Bitangent mgl32.Vec3
}

func NewPointLight(xmlLight config.XmlLight) *PointLight {
	l := &PointLight{
		Position:         xmlLight.XMLPosition.XYZW(),
		Color:            xmlLight.XMLColor.RGB(),
		DiffuseColor:     xmlLight.XMLLightDiffuse.XMLColor.RGB(),
		DiffuseIntensity: xmlLight.XMLLightDiffuse.XMLIntensity,
		AmbientIntensity: xmlLight.XMLLightAmbient.XMLIntensity,
		SpecularColor:    xmlLight.XMLLightSpecular.XMLColor.RGB(),
		Atten: &Attenuation{
			Constant: 1.0,
			Linear:   0.007,
			Exp:      0.0002,
		},
		model: mgl32.Ident4(),
	}

	// Atten参数参考表
	// Distance	Constant    Linear    Quadratic
	// 200	    1.0	        0.022      0.0019
	// 325	    1.0	        0.014      0.0007
	// 600	    1.0	        0.007      0.0002

	l.Meshes = mesh.NewMeshPoint([]mgl32.Vec3{l.Position.Vec3()}...)
	for _, m := range l.Meshes {
		for i := range m.Vertices {
			m.Vertices[i].Color[0] = l.Color.X()
			m.Vertices[i].Color[1] = l.Color.Y()
			m.Vertices[i].Color[2] = l.Color.Z()
		}

		m.Dispose()
		m.Setup()
	}

	l.Init()
	return l
}

func (l *PointLight) Init() {
	l.shader = &shader.Shader{VertFilePath: "./resource/shader/light.vert", FragFilePath: "./resource/shader/light.frag"}
	if err := l.shader.Init(); err != nil {
		logger.Error(err)
		return
	}

	program := l.shader.Use()

	// Shader
	l.projectionUniform = gl.GetUniformLocation(program, gl.Str("projection\x00"))
	l.viewUniform = gl.GetUniformLocation(program, gl.Str("view\x00"))
	l.modelUniform = gl.GetUniformLocation(program, gl.Str("model\x00"))
	l.colorUniform = gl.GetUniformLocation(program, gl.Str("lightColor\x00"))

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))
}

func (l *PointLight) SetPosition(p mgl32.Vec4) {
	l.Position = p
}

func (l *PointLight) SetAmbientIntensity(f float32) {
	l.AmbientIntensity = f
}

func (l *PointLight) SetDiffuseIntensity(f float32) {
	l.DiffuseIntensity = f
}

func (l *PointLight) SetDiffuseColor(r, g, b float32) {
	l.DiffuseColor = mgl32.Vec3{r, g, b}
}

func (l *PointLight) SetSpecularColor(r, g, b float32) {
	l.SpecularColor = mgl32.Vec3{r, g, b}
}

func (l *PointLight) SetAttenuation(rang, constant, linear, exp float32) {
	l.Atten.Range = rang
	l.Atten.Constant = constant
	l.Atten.Linear = linear
	l.Atten.Exp = exp
}

func (l *PointLight) Update(elapsed float64) {
	l.model = l.model.Mul4(mgl32.HomogRotate3DY(float32(elapsed)))
	l.Position = mgl32.HomogRotate3DY(float32(elapsed)).Mul4x1(l.Position)
}

func (l *PointLight) Render(projection mgl32.Mat4, view mgl32.Mat4, model mgl32.Mat4) {
	model = model.Mul4(l.model)

	// Shader
	program := l.shader.Program
	gl.UseProgram(program)

	gl.UniformMatrix4fv(l.projectionUniform, 1, false, &(projection[0]))
	gl.UniformMatrix4fv(l.viewUniform, 1, false, &(view[0]))
	gl.UniformMatrix4fv(l.modelUniform, 1, false, &(model[0]))

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))

	gl.PointSize(10)
	for _, m := range l.Meshes {
		m.Draw(program)
	}

	gl.UseProgram(0)

}
