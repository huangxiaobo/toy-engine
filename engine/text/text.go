package text

import (
	"bufio"
	"fmt"
	"github.com/golang/freetype"
	"golang.org/x/image/font"
	"golang.org/x/image/math/fixed"
	"image"
	"image/color"
	"image/draw"
	"image/png"
	"io/ioutil"
	"log"
	"os"
	"toy/engine/config"
	"toy/engine/logger"
	"unicode/utf8"

	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/golang/freetype/truetype"
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

const FontSize = 64
const FontDpi = 72
const Spacing = 5

type Character struct {
	Tid     uint32
	Size    mgl32.Vec2
	Bearing mgl32.Vec2
	Advance int32
}

type Text struct {
	loaded  bool
	vao     uint32
	vbo     *VboData
	pix     float32
	Texture *texture.Texture
	shader  *shader.Shader

	text       string
	characters []*Character
}

func (f *Text) Init() {
	f.vbo = &VboData{}
	f.text = "Toy引擎"

	f.characters = make([]*Character, 0)
}

var rulerR = color.RGBA{R: 0xff, G: 0x22, B: 0x22, A: 0xff}
var rulerG = color.RGBA{R: 0x22, G: 0xff, B: 0x22, A: 0xff}
var rulerB = color.RGBA{R: 0x22, G: 0x22, B: 0xff, A: 0xff}

func DrawRectangle(rgba *image.RGBA, rect Rectangle) {
	for i := rect.Min.X - 1; i <= rect.Max.X+1; i++ {
		rgba.Set(i, rect.Min.Y, rulerB)
		rgba.Set(i, rect.Max.Y, rulerB)
	}

	for i := rect.Min.Y - 1; i <= rect.Max.Y+1; i++ {
		rgba.Set(rect.Min.X, i, rulerG)
		rgba.Set(rect.Max.X, i, rulerG)
	}
}

func DrawLineX(rgba *image.RGBA, x int) {
	for i := 0; i <= 1024; i++ {
		rgba.Set(x, i, rulerR)
	}
}

func DrawLineY(rgba *image.RGBA, y int) {
	for i := 0; i <= 1024; i++ {
		rgba.Set(i, y, rulerR)
	}
}

type Rectangle struct {
	Min struct {
		X int
		Y int
	}
	Max struct {
		X int
		Y int
	}
}

func (r *Rectangle) String() string {
	return fmt.Sprintf("Rect{min:(%d,%d), max:（%d，%d}", r.Min.X, r.Min.Y, r.Max.X, r.Max.Y)
}

// Load a TrueType font from a file and generate a texture
// with all important characters.
func (f *Text) Load(path string, pix float32) {
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
	size := nextP2(int(pix * FontSize))
	fg, bg := image.White, image.Black
	rgba := image.NewRGBA(image.Rect(0, 0, size, size))

	draw.Draw(rgba, rgba.Bounds(), bg, image.ZP, draw.Src)
	ft := freetype.NewContext()
	ft.SetDPI(FontDpi)
	ft.SetFont(fontFace)
	ft.SetFontSize(FontSize)
	ft.SetClip(rgba.Bounds())
	ft.SetDst(rgba)
	ft.SetSrc(fg)
	ft.SetHinting(font.HintingFull)

	// TrueType stuff
	opts := truetype.Options{}
	opts.Size = FontSize
	face := truetype.NewFace(fontFace, &opts)

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
	origin := fixed.P(0, FontSize+100)

	charIdx := 0
	for charIdx < len(f.text) {
		c, n := utf8.DecodeRuneInString(f.text[charIdx:])
		charIdx += n

		bound, _, _ := face.GlyphBounds(c)
		log.Printf("GlyphBounds of %s is %+v\n", string(c), bound)

		dr, advance, _ := face.GlyphBounds(c)

		origin.X -= dr.Min.X
		_, err := ft.DrawString(string(c), origin)
		if err != nil {
			log.Println(err)
			return
		}

		//    ------------------------->
		//    |
		//    |
		//    |
		//    |
		//   \|/
		// 字体从上往下，从左往右，根据orign进行移动，并且根据y=0进行翻转
		var vertRect = Rectangle{}
		vertRect.Min.X = int(dr.Min.X >> 6)
		vertRect.Min.Y = int(dr.Min.Y >> 6)
		vertRect.Max.X = int(dr.Max.X >> 6)
		vertRect.Max.Y = int(dr.Max.Y >> 6)

		// 移动
		vertRect.Min.X += int(origin.X >> 6)
		vertRect.Max.X += int(origin.X >> 6)
		// 翻转
		vertRect.Min.Y, vertRect.Max.Y = -vertRect.Max.Y, -vertRect.Min.Y
		fmt.Printf("image rect >>>>>>>>>>>>> %s\n", vertRect.String())

		// 纹理根据origin进行移动，需要同时X和Y进行移动
		var imageRect = Rectangle{}
		imageRect.Min.X = int(dr.Min.X>>6 + origin.X>>6)
		imageRect.Min.Y = int(dr.Min.Y>>6 + origin.Y>>6)
		imageRect.Max.X = int(dr.Max.X>>6 + origin.X>>6)
		imageRect.Max.Y = int(dr.Max.Y>>6 + origin.Y>>6)
		fmt.Printf("image rect >>>>>>>>>>>>> %s\n", imageRect.String())

		verQuads := []float32{
			float32(vertRect.Min.X), float32(vertRect.Min.Y),
			float32(vertRect.Min.X), float32(vertRect.Max.Y),
			float32(vertRect.Max.X), float32(vertRect.Min.Y),
			float32(vertRect.Max.X), float32(vertRect.Max.Y),
		}
		// Texture coordinates (normalized)
		texQuads := []float32{
			float32(imageRect.Min.X) / float32(size), float32(imageRect.Max.Y) / float32(size),
			float32(imageRect.Min.X) / float32(size), float32(imageRect.Min.Y) / float32(size),
			float32(imageRect.Max.X) / float32(size), float32(imageRect.Max.Y) / float32(size),
			float32(imageRect.Max.X) / float32(size), float32(imageRect.Min.Y) / float32(size),
		}

		fmt.Printf("verQuads: %+v\n", verQuads)
		fmt.Printf("texQuads: %+v\n", texQuads)
		for v := 0; v < 8; v += 2 {
			vQuads, vTexQuads := verQuads[v:(v+2)], texQuads[v:(v+2)]

			// Data is like (X, Y, U, V)
			f.vbo.AppendData(vQuads)
			f.vbo.AppendData(vTexQuads)
		}

		origin.X += advance
		origin.X += Spacing << 6

	}
	for _, ch := range f.characters {
		fmt.Printf("%f\n ", ch.Size.X())
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
func (f *Text) Render(x, y int, color mgl32.Vec4) {
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

	width := float32(config.Config.WindowWidth)
	height := float32(config.Config.WindowHeight)
	projection := mgl32.Ortho2D(0, width, 0, height)

	f.shader.Use()
	f.shader.SetUniform("projection", projection)
	f.shader.SetUniform("color", color)

	f.Texture.Bind()

	var scale float32 = 1.0

	for i := 0; i < utf8.RuneCount([]byte(f.text)); i += 1 {
		model := mgl32.Ident4().Mul4(mgl32.Scale3D(scale, scale, 0))

		model = model.Add(mgl32.Translate3D(float32(x), float32(y), 0))
		f.shader.SetUniform("model", model)

		gl.DrawArrays(gl.TRIANGLE_STRIP, int32((i)*4), 4)
	}

	gl.Enable(gl.DEPTH_TEST)
	gl.Disable(gl.BLEND)
}
