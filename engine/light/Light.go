package light

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"

	"toy/engine/logger"
	"toy/engine/shader"
)

type Light struct {
	Position  mgl32.Vec4
	Direction mgl32.Vec3
	Color     mgl32.Vec3

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

func (l *Light) Init() {
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

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))

	// Configure the vertex data
	gl.GenVertexArrays(1, &l.vao)
	gl.BindVertexArray(l.vao)

	// vert buff
	gl.GenBuffers(1, &l.vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, l.vbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(vertices)*4, gl.Ptr(&vertices[0]), gl.STATIC_DRAW)
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	// // Get an index for the attribute from the shader
	l.locVert = uint32(gl.GetAttribLocation(program, gl.Str("position\x00")))

	// Unbind the buffer
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)
	gl.DisableVertexAttribArray(0)
	gl.BindVertexArray(0)
}

func (l *Light) Update(elapsed float64) {
}

func (l *Light) Render(projection mgl32.Mat4, view mgl32.Mat4, model mgl32.Mat4) {

	program := l.shader.Program
	// Shader
	gl.UseProgram(program)

	gl.UniformMatrix4fv(l.projectionUniform, 1, false, &projection[0])
	gl.UniformMatrix4fv(l.viewUniform, 1, false, &view[0])
	gl.UniformMatrix4fv(l.modelUniform, 1, false, &model[0])

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))

	// 开启顶点数组
	gl.BindVertexArray(l.vao)

	gl.BindBuffer(gl.ARRAY_BUFFER, l.vbo)
	gl.EnableVertexAttribArray(l.locVert)
	gl.VertexAttribPointer(
		l.locVert, // attribute index
		3,         // number of elements per vertex, here (x,y,z)
		gl.FLOAT,  // the type of each element
		false,     // take our values as-is
		0,         // no extra data between each position
		nil,       // offset of first element
	)

	gl.PointSize(10)
	gl.DrawArrays(gl.POINTS, 0, 1)

	gl.UseProgram(0)

	// Unbind the buffer
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)
	gl.DisableVertexAttribArray(0)
	gl.BindVertexArray(0)
}
