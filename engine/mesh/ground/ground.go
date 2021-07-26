package ground

import (
	"github.com/go-gl/gl/v4.1-core/gl"

	"toy/engine"
	"toy/engine/mesh"
)

type Ground struct {
	mesh.WavefrontObject
}

func (g *Ground) Init(w *engine.World) {
	g.WavefrontObject.Init(w)
}

func (g *Ground) Update(elapsed float64) {

}

func (g *Ground) Render(w *engine.World) {
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)
	g.WavefrontObject.Render(w)
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
}
