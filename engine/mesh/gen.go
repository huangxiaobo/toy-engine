package mesh

import (
	"toy/engine"
)

/*
        v2            v3
        -------------.
       /            /
      /            /
     /            /
    /            /
   .------------.
   v0           v1

	(V0， V1， V2), (V2，V1， V2)
*/

func GenGroundMeshData() *engine.MeshData {
	meshData := &engine.MeshData{}

	xn := 20
	zn := 20
	for zi := -zn; zi <= zn; zi++ {
		for xi := -xn; xi <= xn; xi++ {
			x := float32(xi)
			y := float32(0.0)
			z := float32(zi)

			meshData.Vertices = append(meshData.Vertices, x)
			meshData.Vertices = append(meshData.Vertices, y)
			meshData.Vertices = append(meshData.Vertices, z)

			meshData.Uvs = append(meshData.Uvs, 0.0)
			meshData.Uvs = append(meshData.Uvs, 0.0)

			meshData.Normals = append(meshData.Normals, 0.0)
			meshData.Normals = append(meshData.Normals, 1.0)
			meshData.Normals = append(meshData.Normals, 0.0)
		}
	}

	size := 2*xn + 1
	for zi := 0; zi < size-1; zi++ {
		for xi := 0; xi < size-1; xi++ {
			v0 := zi*size + xi
			v1 := v0 + 1
			v2 := v0 + size
			v3 := v2 + 1

			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v0))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v1))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v2))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v2))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v1))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v3))
		}
	}

	return meshData
}
