package engine

import (
	"bufio"
	"fmt"
	"image"
	"image/draw"
	"image/png"
	"io/ioutil"
	"log"
	"os"
	"toy/engine/logger"

	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/golang/freetype/truetype"
	"golang.org/x/image/font"
	"golang.org/x/image/math/fixed"

	"toy/engine/shader"
	"toy/engine/texture"
)

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

func nextP2(a int) int {
	var rval = 1
	for rval < a {
		rval <<= 1
	}
	return rval
}

type Font struct {
	loaded  bool
	vao     uint32
	vbo     *VboData
	pix     float32
	Texture *texture.Texture
	shader  *shader.Shader

	text string
}

func (f *Font) Init() {
	f.vbo = &VboData{}
	f.text = "Welcome"
}

// Load a TrueType font from a file and generate a texture
// with all important characters.
func (f *Font) Load(path string, pix float32) {
	contents, err := ioutil.ReadFile(path)
	if err != nil {
		fmt.Println("Could not read font file: " + path)
		panic(err)
	}
	fontFace, err := truetype.Parse(contents)
	if err != nil {
		fmt.Println("Could not parse font file: " + path)
		panic(err)
	}

	// Create a texture for the characters
	// Find the next power of 2 for the texture size
	size := nextP2(int(pix * 16))
	fg, bg := image.White, image.Transparent
	rgba := image.NewRGBA(image.Rect(0, 0, size, size))

	draw.Draw(rgba, rgba.Bounds(), bg, image.ZP, draw.Src)
	d := &font.Drawer{
		Dst: rgba,
		Src: fg,
		Face: truetype.NewFace(fontFace, &truetype.Options{
			Size:    float64(pix),
			DPI:     72,
			Hinting: font.HintingNone,
		}),
	}

	f.pix = pix
	// Some GL preps
	gl.GenVertexArrays(1, &f.vao)
	gl.BindVertexArray(f.vao)
	f.vbo.Create()
	f.vbo.Bind()

	f.shader = &shader.Shader{VertFilePath: "./resource/font/font.vert", FragFilePath: "./resource/font/font.frag"}
	f.shader.Init()
	f.shader.Use()
	f.shader.SetUniform("tex", 0)

	// Create vertex data (and coordinates in the texture) for each character
	for i, v := range f.text {
		c := string(v)
		x, y := i%16, i/16

		// Draw the character on the texture
		d.Dot = fixed.P(x*int(pix), (y+1)*int(pix))
		d.DrawString(c)

		// Vertices
		quads := []float32{
			0, 0,
			0, pix,
			pix, 0,
			pix, pix,
		}

		// 字体宽和高不相等
		normX := func(n int) float32 {
			return float32(n) / 16.0
		}
		normY := func(n int) float32 {
			return float32(n) / 12.0
		}

		// Texture coordinates (normalized)
		texQuads := []float32{
			normX(x), normY(y + 1),
			normX(x), normY(y),
			normX(x + 1), normY(y + 1),
			normX(x + 1), normY(y),
		}

		for v := 0; v < 8; v += 2 {
			vQuads, vTexQuads := quads[v:(v+2)], texQuads[v:(v+2)]

			// Data is like (X, Y, U, V)
			f.vbo.AppendData(vQuads)
			f.vbo.AppendData(vTexQuads)
		}
	}

	// Save that RGBA image to disk.
	outFile, err := os.Create("./output/out.png")
	if err != nil {
		log.Println(err)
		os.Exit(1)
	}
	defer outFile.Close()
	b := bufio.NewWriter(outFile)
	err = png.Encode(b, rgba)
	if err != nil {
		log.Println(err)
		os.Exit(1)
	}
	err = b.Flush()
	if err != nil {
		log.Println(err)
		os.Exit(1)
	}
	fmt.Println("Wrote out.png OK.")

	// Upload data to GPU and we're done
	f.Texture = texture.NewTextureFromRGBA(rgba)
	f.Texture.Bind()
	f.Texture.SetGLParam(gl.TEXTURE_MIN_FILTER, gl.LINEAR)
	f.Texture.SetGLParam(gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	f.Texture.Upload()

	f.vbo.UploadData(gl.STATIC_DRAW)

	positionUniform := gl.GetUniformLocation(f.shader.Program, gl.Str("position\x00"))
	texcoordUniform := gl.GetUniformLocation(f.shader.Program, gl.Str("texcoord\x00"))

	gl.EnableVertexAttribArray(uint32(positionUniform))
	gl.VertexAttribPointer(uint32(positionUniform), 2, gl.FLOAT, false, 4*4, gl.PtrOffset(0))
	gl.EnableVertexAttribArray(uint32(texcoordUniform))
	gl.VertexAttribPointer(uint32(texcoordUniform), 2, gl.FLOAT, false, 4*4, gl.PtrOffset(2*4))

	f.loaded = true
}

// Render a text using the font
func (f *Font) Render(text string, x, y int, pix float32, color mgl32.Vec4) {
	if !f.loaded {
		return
	}

	gl.Disable(gl.DEPTH_TEST)
	gl.Enable(gl.BLEND)
	gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
	gl.BindVertexArray(f.vao)

	f.vbo.Bind()
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

	f.shader.Use()
	f.shader.SetUniform("projection", mgl32.Ortho2D(0, WindowWidth, 0, WindowHeight))
	f.shader.SetUniform("color", color)

	f.Texture.Bind()

	scale := pix / f.pix
	for i := 0; i < len(f.text); i++ {
		model := mgl32.Ident4().Mul4(mgl32.Scale3D(scale, scale, 0))

		model = model.Add(mgl32.Translate3D(float32(x)+float32(i)*pix, float32(y), 0))
		f.shader.SetUniform("model", model)

		gl.DrawArrays(gl.TRIANGLE_STRIP, int32((i)*4), 4)
	}

	gl.Enable(gl.DEPTH_TEST)
	gl.Disable(gl.BLEND)
}
