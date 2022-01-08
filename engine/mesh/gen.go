package mesh

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

func GenGroundMeshData() *WavefrontMesh {
	meshData := &WavefrontMesh{}

	x_num := 10
	x_strip := 2
	z_num := 10
	z_strip := 2

	for zi := -z_num; zi <= z_num; zi += 1 {
		for xi := -x_num; xi <= x_num; xi += 1 {
			x := float32((xi) * x_strip)
			y := float32(0.0)
			z := float32((zi) * z_strip)

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

	x_row_num := 2*x_num + 1
	z_row_num := 2*z_num + 1

	for zi := 0; zi < z_row_num-1; zi++ {
		for xi := 0; xi < x_row_num-1; xi++ {
			v0 := zi*x_row_num + xi
			v1 := v0 + 1
			v2 := v0 + x_row_num
			v3 := v2 + 1

			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v0))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v2))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v1))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v1))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v2))
			meshData.VertexIndices = append(meshData.VertexIndices, uint16(v3))

		}
	}

	return meshData
}

func GenCubeMeshData(size float32) *WavefrontMesh {
	meshData := &WavefrontMesh{}

	vertices := []float32{
		-1.0, +1.0, +1.0, // v0
		-1.0, +1.0, -1.0, // v1
		+1.0, +1.0, -1.0, // v2
		+1.0, +1.0, +1.0, // v3

		-1.0, -1.0, +1.0, // v0'
		-1.0, -1.0, -1.0, // v1'
		+1.0, -1.0, -1.0, // v2'
		+1.0, -1.0, +1.0, // v3'
	}

	for i := 0; i <= 8*3; i += 3 {
		x := vertices[i+0]
		y := vertices[i+1]
		z := vertices[i+2]

		meshData.Vertices = append(meshData.Vertices, x)
		meshData.Vertices = append(meshData.Vertices, y)
		meshData.Vertices = append(meshData.Vertices, z)

		meshData.Uvs = append(meshData.Uvs, 0.0)
		meshData.Uvs = append(meshData.Uvs, 0.0)

		meshData.Normals = append(meshData.Normals, 0.0)
		meshData.Normals = append(meshData.Normals, 1.0)
		meshData.Normals = append(meshData.Normals, 0.0)
	}

	meshData.VertexIndices = append(meshData.VertexIndices, uint16(0))
	meshData.VertexIndices = append(meshData.VertexIndices, uint16(1))
	meshData.VertexIndices = append(meshData.VertexIndices, uint16(2))
	meshData.VertexIndices = append(meshData.VertexIndices, uint16(2))
	meshData.VertexIndices = append(meshData.VertexIndices, uint16(1))
	meshData.VertexIndices = append(meshData.VertexIndices, uint16(3))

	return meshData
}
