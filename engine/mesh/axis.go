package mesh

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"toy/engine"
	"toy/engine/config"
	"toy/engine/logger"
	"toy/engine/shader"
)

var axisVertices = []float32{
	//  X, Y, Z,
	0.0, 0.0, 0.0,
	10.0, 0.0, 0.0,

	0.0, 0.0, 0.0,
	0.0, 10.0, 0.0,

	0.0, 0.0, 0.0,
	0.0, 0.0, 10.0,
}

type Axis struct {
	Name       string
	VsFilePath string
	FsFilePath string

	model mgl32.Mat4

	shader            *shader.Shader
	projectionUniform int32
	viewUniform       int32
	modelUniform      int32
	vao               uint32
	vbo               uint32

	vertAttrib uint32
}

func (axis *Axis) Init(w *engine.World) {
	axis.shader = &shader.Shader{VertFilePath: axis.VsFilePath, FragFilePath: axis.FsFilePath}
	if err := axis.shader.Init(); err != nil {
		logger.Error(err)
		return
	}

	program := axis.shader.Use()

	// Shader
	axis.projectionUniform = gl.GetUniformLocation(program, gl.Str("projection\x00"))
	axis.viewUniform = gl.GetUniformLocation(program, gl.Str("view\x00"))
	axis.modelUniform = gl.GetUniformLocation(program, gl.Str("model\x00"))

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))

	// Configure the vertex data
	gl.GenVertexArrays(1, &axis.vao)
	gl.BindVertexArray(axis.vao)

	// vert buff
	gl.GenBuffers(1, &axis.vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, axis.vbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(axisVertices)*4, gl.Ptr(&axisVertices[0]), gl.STATIC_DRAW)
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	//// Get an index for the attribute from the shader
	axis.vertAttrib = uint32(gl.GetAttribLocation(program, gl.Str("position\x00")))

	// Unbind the buffer
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)
	gl.EnableVertexAttribArray(0)
	gl.BindVertexArray(0)

	axis.model = mgl32.Ident4()
}

func (axis *Axis) Update(elapsed float64) {
}

func (axis *Axis) Render(w *engine.World) {
	// Render
	width := float32(config.Config.WindowWidth)
	height := float32(config.Config.WindowHeight)
	projection := mgl32.Perspective(
		mgl32.DegToRad(w.Camera.Zoom),
		width/height,
		0.1,
		100.0,
	)
	view := w.Camera.GetViewMatrix()

	program := axis.shader.Program
	// Shader
	gl.UseProgram(program)

	gl.UniformMatrix4fv(axis.projectionUniform, 1, false, &projection[0])
	gl.UniformMatrix4fv(axis.viewUniform, 1, false, &view[0])
	gl.UniformMatrix4fv(axis.modelUniform, 1, false, &axis.model[0])

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))

	// 开启顶点数组
	gl.BindVertexArray(axis.vao)

	gl.BindBuffer(gl.ARRAY_BUFFER, axis.vbo)
	gl.EnableVertexAttribArray(axis.vertAttrib)
	gl.VertexAttribPointer(
		axis.vertAttrib, // attribute index
		3,               // number of elements per vertex, here (x,y,z)
		gl.FLOAT,        // the type of each element
		false,           // take our values as-is
		0,               // no extra data between each position
		nil,             // offset of first element
	)
	gl.LineWidth(10)
	gl.PointSize(10)

	gl.DrawArrays(gl.LINES, 0, 6)

	gl.UseProgram(0)
	//gl.BindBuffer(gl.ARRAY_BUFFER, 0)
	//gl.EnableVertexAttribArray(0)
	//gl.BindVertexArray(0)
}