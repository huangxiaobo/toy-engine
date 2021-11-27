package ground

import (
	// "github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/gl/v4.1-core/gl"

	"github/huangxiaobo/toy-engine/engine"
	"github/huangxiaobo/toy-engine/engine/mesh"
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
	// gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
	g.WavefrontObject.Render(w)
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
}
