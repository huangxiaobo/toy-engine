package ground

import (
	"github.com/go-gl/gl/v4.1-core/gl"

	"toy/engine"
	"toy/engine/mesh"
)

type Ground struct {
	mesh.WavefrontObject
}

func (wfo *Ground) Init(w *engine.World) {
	wfo.WavefrontObject.Init(w)
	wfo.DrawMode = gl.LINES
}

func (wfo *Ground) Update(elapsed float64) {

}
