package light

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"

	"github/huangxiaobo/ToyEngine/engine/logger"
	"github/huangxiaobo/ToyEngine/engine/shader"
)

type Attenuation struct {
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
	Atten            Attenuation

	shader            *shader.Shader
	projectionUniform int32
	viewUniform       int32
	modelUniform      int32
	vao               uint32
	vbo               uint32

	locVert uint32
}

var vertices = []float32{
	//  X, Y, Z,
	0.0, 0.0, 0.0,
}

func (light *PointLight) Init() {
	light.shader = &shader.Shader{VertFilePath: "./resource/shader/light.vert", FragFilePath: "./resource/shader/light.frag"}
	if err := light.shader.Init(); err != nil {
		logger.Error(err)
		return
	}

	program := light.shader.Use()

	// Shader
	light.projectionUniform = gl.GetUniformLocation(program, gl.Str("projection\x00"))
	light.viewUniform = gl.GetUniformLocation(program, gl.Str("view\x00"))
	light.modelUniform = gl.GetUniformLocation(program, gl.Str("model\x00"))

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))

	// Configure the vertex data
	gl.GenVertexArrays(1, &light.vao)
	gl.BindVertexArray(light.vao)

	// vert buff
	gl.GenBuffers(1, &light.vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, light.vbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(vertices)*4, gl.Ptr(&vertices[0]), gl.STATIC_DRAW)
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	// // Get an index for the attribute from the shader
	light.locVert = uint32(gl.GetAttribLocation(program, gl.Str("position\x00")))

	// Unbind the buffer
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)
	gl.DisableVertexAttribArray(0)
	gl.BindVertexArray(0)

	light.SetDiffuseColor(1.0, .5, 0.0)   // color the light orange
	light.SetSpecularColor(1.0, 1.0, 0.0) // yellow highlights
	light.DiffuseIntensity = 0.25
	light.AmbientIntensity = 0.25
	// light.SetAttenuation(100, 1.0, 0.045, 0.0075);
}

func (light *PointLight) SetDiffuseColor(r, g, b float32) {
	light.DiffuseColor = mgl32.Vec3{r, g, b}
}

func (light *PointLight) SetSpecularColor(r, g, b float32) {
	light.SpecularColor = mgl32.Vec3{r, g, b}
}

func (light *PointLight) Update(elapsed float64) {
}

func (light *PointLight) Render(projection mgl32.Mat4, view mgl32.Mat4, model mgl32.Mat4) {

	program := light.shader.Program
	// Shader
	gl.UseProgram(program)

	gl.UniformMatrix4fv(light.projectionUniform, 1, false, &projection[0])
	gl.UniformMatrix4fv(light.viewUniform, 1, false, &view[0])
	gl.UniformMatrix4fv(light.modelUniform, 1, false, &model[0])

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))

	// 开启顶点数组
	gl.BindVertexArray(light.vao)

	gl.BindBuffer(gl.ARRAY_BUFFER, light.vbo)
	gl.EnableVertexAttribArray(light.locVert)
	gl.VertexAttribPointer(
		light.locVert, // attribute index
		3,             // number of elements per vertex, here (x,y,z)
		gl.FLOAT,      // the type of each element
		false,         // take our values as-is
		0,             // no extra data between each position
		nil,           // offset of first element
	)

	gl.PointSize(10)
	gl.DrawArrays(gl.POINTS, 0, 1)

	gl.UseProgram(0)

	// Unbind the buffer
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)
	gl.DisableVertexAttribArray(0)
	gl.BindVertexArray(0)
}
