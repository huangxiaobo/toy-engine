package text

import (
	"fmt"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/veandco/go-sdl2/sdl"
	"github.com/veandco/go-sdl2/ttf"

	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/shader"
	"github.com/huangxiaobo/toy-engine/engine/texture"
)

type VaoData struct {
	aid uint32
}

func (vao *VaoData) Create() {
	gl.GenVertexArrays(1, &vao.aid)
	gl.BindVertexArray(vao.aid)
}

func (vao *VaoData) Bind() {
	gl.BindVertexArray(vao.aid)
}

func (vao *VaoData) UnBind() {
	gl.BindVertexArray(0)
}

type VboData struct {
	bid  uint32
	data []float32
}

func (v *VboData) Create() {
	gl.GenBuffers(1, &v.bid)
	gl.BindBuffer(gl.ARRAY_BUFFER, v.bid)
	if len(v.data) > 0 {
		gl.BufferData(gl.ARRAY_BUFFER, len(v.data)*4, gl.Ptr(&v.data[0]), gl.STATIC_DRAW)
	}

	gl.BindBuffer(gl.ARRAY_BUFFER, 0)
}

func (v *VboData) Bind() {
	gl.BindBuffer(gl.ARRAY_BUFFER, v.bid)
}

func (v *VboData) AppendData(d []float32) {
	v.data = append(v.data, d...)
}

func (v *VboData) UploadData(val uint32) {
	logger.Info(val)
	gl.BufferData(gl.ARRAY_BUFFER, len(v.data)*4, gl.Ptr(&v.data[0]), gl.STATIC_DRAW)
	fmt.Printf("data: %v\n", v.data)
}

func nextP2(a int32) int32 {
	var rval int32 = 1
	for rval < a {
		rval <<= 1
	}
	return rval
}

type Text struct {
	loaded bool
	vao    *VaoData
	vbo    *VboData
	pix    float32
	tex    *texture.Texture
	shader *shader.Shader

	content string
}

func NewText(content string, size int) (*Text, error) {

	surface, err := creatSurface(content, size)
	if err != nil {
		return nil, err
	}
	w, h := surface.W, surface.H

	tex := texture.NewTextureFromSDLSurface(surface)

	shader := &shader.Shader{VertFilePath: "./resource/font/font.vert", FragFilePath: "./resource/font/font.frag"}
	shader.Init()
	shader.Use()

	vao := &VaoData{}
	vao.Create()

	vbo := &VboData{}
	vbo.Create()
	vbo.Bind()

	vbo.AppendData([]float32{0, 0, 0.0, 1.0})
	vbo.AppendData([]float32{float32(0 + w), 0.0, 1.0, 1.0})
	vbo.AppendData([]float32{0.0, float32(0.0 + h), 0.0, 0.0})
	vbo.AppendData([]float32{float32(0.0 + w), float32(0.0 + h), 1.0, 0.0})

	vbo.UploadData(gl.STATIC_DRAW)

	positionUniform := gl.GetUniformLocation(shader.Program, gl.Str("position\x00"))
	texcoordUniform := gl.GetUniformLocation(shader.Program, gl.Str("texcoord\x00"))

	gl.EnableVertexAttribArray(uint32(positionUniform))
	gl.VertexAttribPointer(uint32(positionUniform), 2, gl.FLOAT, false, 4*4, gl.PtrOffset(0))
	gl.EnableVertexAttribArray(uint32(texcoordUniform))
	gl.VertexAttribPointer(uint32(texcoordUniform), 2, gl.FLOAT, false, 4*4, gl.PtrOffset(2*4))

	return &Text{
		vao:     vao,
		vbo:     vbo,
		shader:  shader,
		tex:     tex,
		content: content,
		loaded:  true,
	}, nil
}

func creatSurface(content string, size int) (*sdl.Surface, error) {
	// Load the font for our text

	fontFile := "./resource/font/微软雅黑.ttf"
	font, err := ttf.OpenFont(fontFile, size)
	if err != nil {
		sdl.Quit()
		return nil, fmt.Errorf("failed to open font: %w", err)
	}
	defer font.Close()

	//var surface *sdl.Surface
	var text *sdl.Surface

	// Create a red text with the font
	if text, err = font.RenderUTF8Blended(content, sdl.Color{R: 255, G: 255, B: 255, A: 255}); err != nil {
		fmt.Println(err)
		return nil, fmt.Errorf("RenderUTF8Blended failed: %w", err)
	}
	defer text.Free()

	w := nextP2(text.W)
	h := nextP2(text.H)

	surface, _ := sdl.CreateRGBSurface(0, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000)
	defer surface.Free()

	if err := text.Blit(nil, surface, &sdl.Rect{W: w, H: h}); err != nil {
		panic(err)
	}

	return surface, nil

}

// Render a text using the font
func (t *Text) Render(x, y int, color mgl32.Vec4) {
	if !t.loaded {
		return
	}

	positionUniform := gl.GetUniformLocation(t.shader.Program, gl.Str("position\x00"))
	texcoordUniform := gl.GetUniformLocation(t.shader.Program, gl.Str("texcoord\x00"))

	gl.EnableVertexAttribArray(uint32(positionUniform))
	gl.VertexAttribPointer(uint32(positionUniform), 2, gl.FLOAT, false, 4*4, gl.PtrOffset(0))
	gl.EnableVertexAttribArray(uint32(texcoordUniform))
	gl.VertexAttribPointer(uint32(texcoordUniform), 2, gl.FLOAT, false, 4*4, gl.PtrOffset(2*4))

	gl.Disable(gl.DEPTH_TEST)
	gl.Enable(gl.BLEND)
	gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)

	t.vao.Bind()
	t.vbo.Bind()
	gl.EnableVertexAttribArray(0)
	gl.VertexAttribPointer(
		0,               // attribute index
		2,               // number of elements per vertex, here (x,y,z)
		gl.FLOAT,        // the type of each element
		false,           // take our values as-is
		4*4,             // no extra data between each position
		gl.PtrOffset(0), // offset of first element
	)
	gl.EnableVertexAttribArray(1)
	gl.VertexAttribPointer(
		1,                 // attribute index
		2,                 // number of elements per vertex, here (x,y,z)
		gl.FLOAT,          // the type of each element
		false,             // take our values as-is
		4*4,               // no extra data between each position
		gl.PtrOffset(2*4), // offset of first element
	)

	width := float32(config.Config.WindowWidth)
	height := float32(config.Config.WindowHeight)
	projection := mgl32.Ortho2D(0, width, 0, height)

	t.shader.Use()
	t.shader.SetUniform("projection", projection)
	t.shader.SetUniform("color", mgl32.Vec4{1.0, 0.0, 0.0, 1.0})
	t.shader.SetUniform("model", mgl32.Ident4())
	t.tex.Bind()

	gl.DrawArrays(gl.TRIANGLE_STRIP, int32((0)*4), 4)

	gl.Enable(gl.DEPTH_TEST)
	gl.Disable(gl.BLEND)
}
