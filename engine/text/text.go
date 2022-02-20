package text

import (
	"fmt"
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/huangxiaobo/toy-engine/engine/material"
	"github.com/huangxiaobo/toy-engine/engine/mesh"
	"github.com/huangxiaobo/toy-engine/engine/shader"
	"github.com/huangxiaobo/toy-engine/engine/technique"
	"github.com/huangxiaobo/toy-engine/engine/texture"
	"github.com/huangxiaobo/toy-engine/engine/utils"
	"github.com/veandco/go-sdl2/sdl"
	"github.com/veandco/go-sdl2/ttf"
)

const (
	FontFile = "./resource/font/微软雅黑.ttf"
)

type Text struct {
	Meshes   []mesh.Mesh
	Material *material.Material
	effect   *technique.BaseTechnique
	shader   *shader.Shader

	pix float32
	tex *texture.Texture

	content string
}

func NewText(content string, size int, color mgl32.Vec3) *Text {
	Meshes := make([]mesh.Mesh, 0)
	m := mesh.Mesh{
		DrawMode: gl.TRIANGLE_STRIP,
	}

	// 纹理
	surface, err := creatSurface(content, size, color)
	if err != nil {
		return nil
	}
	w, h := surface.W, surface.H

	tex := texture.NewTextureFromSDLSurface(texture.TextureMaterial, surface)
	m.Textures = append(m.Textures, *tex)

	// 顶点
	for i, vn := range [][]float32{
		{0, 0, 0.0, 1.0},
		{float32(0 + w), 0.0, 1.0, 1.0},
		{0.0, float32(0.0 + h), 0.0, 0.0},
		{float32(0.0 + w), float32(0.0 + h), 1.0, 0.0},
	} {
		v := mesh.Vertex{
			Position:  mgl32.Vec3{vn[0], vn[1], 0},
			Color:     mgl32.Vec3{0, 0, 0},
			Normal:    mgl32.Vec3{0.0, 1.0, 0.0},
			TexCoords: mgl32.Vec2{vn[2], vn[3]},
			Tangent:   mgl32.Vec3{0.0, 0.0, 0.0},
			Bitangent: mgl32.Vec3{0.0, 0.0, 0.0},
		}

		m.Vertices = append(m.Vertices, v)
		m.Indices = append(m.Indices, uint32(i))
	}
	m.Setup()
	Meshes = append(Meshes, m)

	t := &Text{
		Meshes:  Meshes,
		content: content,
		effect:  &technique.BaseTechnique{},
		Material: &material.Material{
			AmbientColor:  mgl32.Vec3{0, 0, 0},
			DiffuseColor:  mgl32.Vec3{0, 0, 0},
			SpecularColor: mgl32.Vec3{0, 0, 0},
			Shininess:     0,
		},
		shader: &shader.Shader{
			VertFilePath: "./resource/font/font.vert",
			FragFilePath: "./resource/font/font.frag",
		},
	}

	t.Init()

	return t
}

func (t *Text) Init() {
	if err := t.shader.Init(); err != nil {
		logger.Error(err)
		panic(err)
	}
	t.effect.Init(t.shader)

}

func creatSurface(content string, size int, color mgl32.Vec3) (*sdl.Surface, error) {
	// 使用指定字符和颜色创建surface

	font, err := ttf.OpenFont(FontFile, size)
	if err != nil {
		sdl.Quit()
		return nil, fmt.Errorf("failed to open font: %w", err)
	}
	defer font.Close()

	//定义 surface *sdl.Surface
	var surfaceRaw *sdl.Surface
	var surfaceNew *sdl.Surface

	// 指定颜色，渲染字符串
	sdlColor := sdl.Color{R: uint8(color.X() * 255), G: uint8(color.Y() * 255), B: uint8(color.Z() * 255), A: 255}
	if surfaceRaw, err = font.RenderUTF8Blended(content, sdlColor); err != nil {
		fmt.Println(err)
		return nil, fmt.Errorf("RenderUTF8Blended failed: %w", err)
	}
	defer surfaceRaw.Free()

	w := utils.NextP2(surfaceRaw.W)
	h := utils.NextP2(surfaceRaw.H)

	// 创建新的surface，并将纹理拷贝到此surface

	var rmask uint32
	var gmask uint32
	var bmask uint32
	var amask uint32

	switch sdl.BYTEORDER {
	case sdl.LIL_ENDIAN:
		rmask = 0x000000ff
		gmask = 0x0000ff00
		bmask = 0x00ff0000
		amask = 0xff000000
	case sdl.BIG_ENDIAN:
		rmask = 0xff000000
		gmask = 0x00ff0000
		bmask = 0x0000ff00
		amask = 0x000000ff
	}

	surfaceNew, err = sdl.CreateRGBSurface(0, w, h, 32, rmask, gmask, bmask, amask)
	defer surfaceNew.Free()

	if err := surfaceRaw.Blit(nil, surfaceNew, &sdl.Rect{W: w, H: h}); err != nil {
		panic(err)
	}

	return surfaceNew, nil
}

// Render 渲染字符串
func (t *Text) Render(x, y int) {

	gl.Disable(gl.DEPTH_TEST)
	gl.Enable(gl.BLEND)
	gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)

	width := float32(config.Config.WindowWidth)
	height := float32(config.Config.WindowHeight)

	projection := mgl32.Ortho2D(0, width, 0, height)
	view := mgl32.Ident4()
	model := mgl32.Ident4().Mul4(mgl32.Translate3D(float32(x), float32(y), 0))
	eyePosition := mgl32.Vec3{0, 0, 0}

	// Effect
	t.effect.Enable()
	t.effect.SetProjectMatrix(&projection)
	t.effect.SetViewMatrix(&view)
	t.effect.SetModelMatrix(&model)
	t.effect.SetEyeWorldPos(&eyePosition)

	gl.BindFragDataLocation(t.effect.ShaderObj.Program, 0, gl.Str("color\x00"))

	for _, m := range t.Meshes {
		m.Draw(t.effect.ShaderObj.Program)
	}
	t.effect.Disable()

	gl.Enable(gl.DEPTH_TEST)
	gl.Disable(gl.BLEND)
}
