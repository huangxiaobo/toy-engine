package mesh

import "github.com/huangxiaobo/toy-engine/engine/model"

var axisVertices = []float32{
	//  X, Y, Z,
	0.0, 0.0, 0.0,
	10.0, 0.0, 0.0,

	0.0, 0.0, 0.0,
	0.0, 10.0, 0.0,

	0.0, 0.0, 0.0,
	0.0, 0.0, 10.0,
}

type Axis struct {
	model.Mesh
}
