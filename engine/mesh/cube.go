package mesh

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/glfw/v3.2/glfw"
	"github.com/go-gl/mathgl/mgl32"
	"toy/engine"
	"toy/engine/loader"
	"toy/engine/logger"
	"toy/engine/shader"
)

type Cube struct {
	Name        string
	ObjFilePath string
	VsFilePath  string
	FsFilePath  string

	model mgl32.Mat4

	obj               engine.Obj
	shader            *shader.Shader
	projectionUniform int32
	viewUniform       int32
	modelUniform      int32
	vao               uint32
	vbo               uint32
	ebo               uint32
	nbo               uint32
	tbo               uint32
	vertAttrib        uint32
	normalAttrib      uint32
}

func (cube *Cube) Init(w *engine.World) {
	if cube.ObjFilePath == "" {
		cube.ObjFilePath = "./resource/cube/cube.obj"
	}

	if cube.VsFilePath == "" {
		cube.VsFilePath = "./resource/cube/cube.vert"
	}
	if cube.FsFilePath == "" {
		cube.FsFilePath = "./resource/cube/cube.frag"
	}

	if err := loader.LoadObj(cube.ObjFilePath, &cube.obj); err != nil {
		logger.Error(err)
		return
	}

	cube.shader = &shader.Shader{VsFilePath: cube.VsFilePath, FsFilePath: cube.FsFilePath}
	if err := cube.shader.Init(); err != nil {
		logger.Error(err)
		return
	}

	program := cube.shader.Use()

	// Shader
	cube.projectionUniform = gl.GetUniformLocation(program, gl.Str("projection\x00"))
	cube.viewUniform = gl.GetUniformLocation(program, gl.Str("view\x00"))
	cube.modelUniform = gl.GetUniformLocation(program, gl.Str("model\x00"))

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))

	// Configure the vertex data
	gl.GenVertexArrays(1, &cube.vao)
	gl.BindVertexArray(cube.vao)

	// vert buff
	gl.GenBuffers(1, &cube.vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, cube.vbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(cube.obj.Vertices)*4, gl.Ptr(&cube.obj.Vertices[0]), gl.STATIC_DRAW)
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	// normal buff
	gl.GenBuffers(1, &cube.nbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, cube.nbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(cube.obj.Normals)*4, gl.Ptr(&cube.obj.Normals[0]), gl.STATIC_DRAW)
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	// uv buff
	gl.GenBuffers(1, &cube.tbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, cube.tbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(cube.obj.Uvs)*2, gl.Ptr(&cube.obj.Uvs[0]), gl.STATIC_DRAW)
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	// index buff
	gl.GenBuffers(1, &cube.ebo)
	gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, cube.ebo)
	gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, len(cube.obj.VertexIndices)*2, gl.Ptr(&cube.obj.VertexIndices[0]), gl.STATIC_DRAW)
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	//// Get an index for the attribute from the shader
	cube.vertAttrib = uint32(gl.GetAttribLocation(program, gl.Str("position\x00")))
	cube.normalAttrib = uint32(gl.GetAttribLocation(program, gl.Str("normal\x00")))

	// Unbind the buffer
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)
	gl.EnableVertexAttribArray(0)
	gl.BindVertexArray(0)

	cube.model = mgl32.Ident4()
}

func (cube *Cube) Update(elapsed float64) {
	cube.model = mgl32.HomogRotate3DX(float32(elapsed)).Mul4(mgl32.HomogRotate3DY(float32(glfw.GetTime())))
}

func (cube *Cube) Render(w *engine.World) {
	// Render
	projection := mgl32.Perspective(
		mgl32.DegToRad(w.Camera.Zoom),
		engine.WindowWidth/engine.WindowHeight,
		0.1,
		100.0,
	)
	view := w.Camera.GetViewMatrix()

	program := cube.shader.Program
	// Shader
	gl.UseProgram(program)

	gl.UniformMatrix4fv(cube.projectionUniform, 1, false, &projection[0])
	gl.UniformMatrix4fv(cube.viewUniform, 1, false, &view[0])
	gl.UniformMatrix4fv(cube.modelUniform, 1, false, &cube.model[0])

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))
	lightPosAttrib := gl.GetUniformLocation(program, gl.Str("lightpos\x00"))
	gl.Uniform3fv(lightPosAttrib, 1, &w.Light.Position[0])
	lightColorAttrib := gl.GetUniformLocation(program, gl.Str("lightColor\x00"))
	gl.Uniform3fv(lightColorAttrib, 1, &w.Light.Color[0])
	viewPosAttrib := gl.GetUniformLocation(program, gl.Str("viewPos\x00"))
	gl.Uniform3fv(viewPosAttrib, 1, &w.Camera.Position[0])

	// 开启顶点数组
	gl.BindVertexArray(cube.vao)

	gl.BindBuffer(gl.ARRAY_BUFFER, cube.vbo)
	gl.EnableVertexAttribArray(cube.vertAttrib)
	gl.VertexAttribPointer(
		cube.vertAttrib, // attribute index
		3,               // number of elements per vertex, here (x,y,z)
		gl.FLOAT,        // the type of each element
		false,           // take our values as-is
		0,               // no extra data between each position
		nil,             // offset of first element
	)

	gl.BindBuffer(gl.ARRAY_BUFFER, cube.nbo)
	gl.EnableVertexAttribArray(cube.normalAttrib)
	gl.VertexAttribPointer(
		cube.normalAttrib, // attribute index
		3,                 // number of elements per vertex, here (x,y,z)
		gl.FLOAT,          // the type of each element
		false,             // take our values as-is
		0,                 // no extra data between each position
		nil,               // offset of first element
	)

	gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, cube.ebo)
	gl.DrawElements(
		gl.TRIANGLES,                       // mode
		int32(len(cube.obj.VertexIndices)), // count
		gl.UNSIGNED_SHORT,                  // type
		nil,                                // element array buffer offset
	)

	gl.PointSize(10)
	gl.DrawElements(
		gl.POINTS,                          // mode
		int32(len(cube.obj.VertexIndices)), // count
		gl.UNSIGNED_SHORT,                  // type
		nil,                                // element array buffer offset
	)
	gl.BindBuffer(gl.ARRAY_BUFFER, 0)
	gl.EnableVertexAttribArray(0)
	gl.BindVertexArray(0)
}
