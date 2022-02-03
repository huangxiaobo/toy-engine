package light

import (
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"unsafe"

	"github.com/huangxiaobo/toy-engine/engine/logger"
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

	Indices  []uint32
	Vertices []Vertex
	vao      uint32
	vbo      uint32
	ebo      uint32
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
	}

	// Atten参数参考表
	// Distance	Constant    Linear    Quadratic
	// 200	    1.0	        0.022      0.0019
	// 325	    1.0	        0.014      0.0007
	// 600	    1.0	        0.007      0.0002

	l.Vertices = []Vertex{
		{
			Position: mgl32.Vec3{0.0, 0.0, 0.0},
			Normal:   mgl32.Vec3{0, 1, 0},
		},
	}
	l.Indices = []uint32{0}

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

	// size of the Vertex struct
	dummy := l.Vertices[0]
	structSize := int(unsafe.Sizeof(dummy))
	structSize32 := int32(structSize)

	// Configure the vertex data
	gl.GenVertexArrays(1, &l.vao)
	gl.GenBuffers(1, &l.vbo)
	gl.GenBuffers(1, &l.ebo)

	gl.BindVertexArray(l.vao)

	// vert buff 复制顶点数组到缓冲中供OpenGL使用
	gl.BindBuffer(gl.ARRAY_BUFFER, l.vbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(l.Vertices)*structSize, gl.Ptr(l.Vertices), gl.STATIC_DRAW)

	// indic buff, 复制索引数组到缓冲中供OpenGL使用
	gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, l.ebo)
	gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, len(l.Indices)*4, gl.Ptr(l.Indices), gl.STATIC_DRAW)

	// Set the vertex attribute pointers
	// Vertex Positions
	gl.VertexAttribPointer(0, 3, gl.FLOAT, false, structSize32, gl.PtrOffset(0))
	gl.EnableVertexAttribArray(0)

	// Vertex Normals
	gl.VertexAttribPointer(1, 3, gl.FLOAT, false, structSize32, unsafe.Pointer(unsafe.Offsetof(dummy.Normal)))
	gl.EnableVertexAttribArray(1)

	// Vertex Texture Coords
	gl.VertexAttribPointer(2, 2, gl.FLOAT, false, structSize32, unsafe.Pointer(unsafe.Offsetof(dummy.TexCoords)))
	gl.EnableVertexAttribArray(2)

	// Vertex Tangent
	gl.EnableVertexAttribArray(3)
	gl.VertexAttribPointer(3, 3, gl.FLOAT, false, structSize32, unsafe.Pointer(unsafe.Offsetof(dummy.Tangent)))
	// Vertex Bitangent
	gl.EnableVertexAttribArray(4)
	gl.VertexAttribPointer(4, 3, gl.FLOAT, false, structSize32, unsafe.Pointer(unsafe.Offsetof(dummy.Bitangent)))

	// Unbind the buffer
	gl.BindVertexArray(0)

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
	l.Position = mgl32.HomogRotate3DY(float32(elapsed)).Mul4x1(l.Position)
}

func (l *PointLight) Render(projection mgl32.Mat4, view mgl32.Mat4, model mgl32.Mat4) {
	model = model.Add(mgl32.Translate3D(l.Position.X(), l.Position.Y(), l.Position.Z()))

	program := l.shader.Program
	// Shader
	gl.UseProgram(program)

	gl.UniformMatrix4fv(l.projectionUniform, 1, false, &projection[0])
	gl.UniformMatrix4fv(l.viewUniform, 1, false, &view[0])
	gl.UniformMatrix4fv(l.modelUniform, 1, false, &model[0])

	gl.Uniform3f(l.colorUniform, l.Color.X(), l.Color.Y(), l.Color.Z())

	gl.BindFragDataLocation(program, 0, gl.Str("color\x00"))

	// 开启顶点数组
	gl.BindVertexArray(l.vao)
	gl.PointSize(10)
	gl.DrawArrays(gl.POINTS, 0, 1)
	gl.BindVertexArray(0)

	gl.UseProgram(0)

}
