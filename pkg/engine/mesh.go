package engine

type Mesh interface {
	Init(w *World)

	Update(elapsed float64)

	Render(w *World)
}

type Obj struct {
	Name        string
	Vertices    []float32
	Coordinates []float32
	Normals     []float32

	VertexIndices []uint16
	UvIndices     []uint16
	NormalIndices []uint16
}
