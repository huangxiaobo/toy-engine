package engine

type Mesh interface {
	Init(w *World)

	Update(elapsed float64)

	Render(w *World)
}

type MeshData struct {
	Name     string
	Vertices []float32
	Uvs      []float32
	Normals  []float32

	VertexIndices []uint16
}
