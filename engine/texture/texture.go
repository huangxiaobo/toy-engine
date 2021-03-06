package texture

import (
	"fmt"
	"github.com/veandco/go-sdl2/sdl"
	_ "golang.org/x/image/bmp"
	"image"
	"image/draw"
	_ "image/gif"
	_ "image/jpeg"
	_ "image/png"
	"os"

	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/kardianos/osext"
)

const (
	TextureMaterial = string("texture_material")
	TextureDiffuse  = string("texture_diffuse")
	TextureSpecular = string("texture_specular")
	TextureNormal   = string("texture_normal")
	TextureHeight   = string("texture_height")
)

type Texture struct {
	Id          uint32
	TextureType string
	Path        string
}

func (tex *Texture) LoadTexture(file string) error {
	imgFile, err := os.Open(file)
	if err != nil {
		// Get the Folder of the current Executable
		dir, err := osext.ExecutableFolder()
		if err != nil {
			return err
		}

		// Read the file and return content or error
		var secondErr error
		imgFile, secondErr = os.Open(fmt.Sprintf("%s/%s", dir, file))
		if secondErr != nil {
			return secondErr
		}
	}

	img, _, err := image.Decode(imgFile)
	if err != nil {
		return err
	}

	rgba := image.NewRGBA(img.Bounds())
	if rgba.Stride != rgba.Rect.Size().X*4 {
		return fmt.Errorf("unsupported stride")
	}
	draw.Draw(rgba, rgba.Bounds(), img, image.Point{X: 0, Y: 0}, draw.Src)

	gl.GenTextures(1, &tex.Id)
	gl.ActiveTexture(gl.TEXTURE0)
	gl.BindTexture(gl.TEXTURE_2D, tex.Id)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)
	gl.TexImage2D(
		gl.TEXTURE_2D,
		0,
		gl.RGBA,
		int32(rgba.Rect.Size().X),
		int32(rgba.Rect.Size().Y),
		0,
		gl.RGBA,
		gl.UNSIGNED_BYTE,
		gl.Ptr(rgba.Pix))

	return nil
}

func (tex *Texture) Bind() {
	gl.BindTexture(gl.TEXTURE_2D, tex.Id)
	gl.ActiveTexture(gl.TEXTURE0)
}

func (tex *Texture) SetGLParam(name uint32, param int32) {
	gl.TexParameteri(gl.TEXTURE_2D, name, param)

}

func (tex *Texture) Upload() {

}

func ImageToPixelData(file string) (*image.RGBA, error) {
	imgFile, err := os.Open(file)
	if err != nil {
		return nil, fmt.Errorf("texture %q not found on disk: %v", file, err)
	}
	defer func(imgFile *os.File) {
		err := imgFile.Close()
		if err != nil {

		}
	}(imgFile)

	img, _, err := image.Decode(imgFile)
	if err != nil {
		return nil, err
	}

	rgba := image.NewRGBA(img.Bounds())
	if rgba.Stride != rgba.Rect.Size().X*4 {
		return nil, fmt.Errorf("unsupported stride")
	}
	draw.Draw(rgba, rgba.Bounds(), img, image.Point{}, draw.Src)
	return rgba, nil
}

func NewTextureFromRGBA(rgba *image.RGBA) *Texture {
	tex := &Texture{}
	gl.GenTextures(1, &tex.Id)
	gl.ActiveTexture(gl.TEXTURE0)
	gl.BindTexture(gl.TEXTURE_2D, tex.Id)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)
	gl.TexImage2D(
		gl.TEXTURE_2D,
		0,
		gl.RGBA,
		int32(rgba.Rect.Size().X),
		int32(rgba.Rect.Size().Y),
		0,
		gl.RGBA,
		gl.UNSIGNED_BYTE,
		gl.Ptr(rgba.Pix))

	return tex
}

func NewTextureFromSDLSurface(texType string, surface *sdl.Surface) *Texture {
	tex := &Texture{
		TextureType: texType,
	}
	gl.GenTextures(1, &tex.Id)
	gl.ActiveTexture(gl.TEXTURE0)
	gl.BindTexture(gl.TEXTURE_2D, tex.Id)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)
	gl.TexImage2D(
		gl.TEXTURE_2D,
		0,
		gl.RGBA,
		surface.W,
		surface.H,
		0,
		gl.RGBA,
		gl.UNSIGNED_BYTE,
		gl.Ptr(surface.Pixels()))

	return tex
}

func NewTexture(texWrapS, texWrapT, texMinFilter, texNagFilter int32, file string) (uint32, error) {
	rgba, _ := ImageToPixelData(file)

	var texture uint32
	gl.GenTextures(1, &texture)
	//gl.ActiveTexture(gl.TEXTURE0)
	gl.BindTexture(gl.TEXTURE_2D, texture)

	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, texWrapS)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, texWrapT)

	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, texMinFilter)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, texNagFilter)

	gl.TexImage2D(
		gl.TEXTURE_2D,
		0,
		gl.RGBA,
		int32(rgba.Rect.Size().X),
		int32(rgba.Rect.Size().Y),
		0,
		gl.RGBA,
		gl.UNSIGNED_BYTE,
		gl.Ptr(rgba.Pix))
	gl.GenerateMipmap(gl.TEXTURE_2D)

	gl.BindTexture(gl.TEXTURE_2D, 0)

	return texture, nil
}
