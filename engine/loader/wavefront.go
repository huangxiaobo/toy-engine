package loader

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"

	"github.com/go-gl/mathgl/mgl32"

	"toy/engine"
	"toy/engine/logger"
)

type Face struct {
	s []string
}

type InterObj struct {
	Name     string
	Vertices []mgl32.Vec3
	Uvs      []mgl32.Vec2
	Normals  []mgl32.Vec3

	Faces []Face
}

func LoadWavefrontObj(objFile string, obj *engine.MeshData) error {
	file, err := os.Open(objFile)
	if err != nil {
		log.Fatal(err)
	}
	defer file.Close()

	interObj := InterObj{}

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		tokens := strings.SplitN(line, " ", 2)

		switch tokens[0] {
		case "v":
			var vec3 mgl32.Vec3
			// v x y z
			cnt, err := fmt.Sscanf(line, "v %f %f %f", &vec3[0], &vec3[1], &vec3[2])
			if cnt != 3 || err != nil {
				logger.Fatal("read vert failed, cnt: %s, err: %s", cnt, err)
			}

			interObj.Vertices = append(interObj.Vertices, vec3)
		case "vt":
			var vec2 mgl32.Vec2
			// vt u v [w]
			cnt, err := fmt.Sscanf(line, "vt %f %f", &vec2[0], &vec2[1])
			if cnt != 2 || err != nil {
				logger.Fatal("read tang failed, cnt: %s, err: %s", cnt, err)
			}
			interObj.Uvs = append(interObj.Uvs, vec2)
		case "vn":
			var vec3 mgl32.Vec3
			// vn x y z
			cnt, err := fmt.Sscanf(line, "vn %f %f %f", &vec3[0], &vec3[1], &vec3[2])
			if cnt != 3 || err != nil {
				logger.Fatal("read normal failed, cnt: %s, err: %s", cnt, err)
			}
			interObj.Normals = append(interObj.Normals, vec3)
		case "f":
			interObj.Faces = append(interObj.Faces, Face{strings.Split(tokens[1], " ")})
		}
	}


	var vmap = make(map[string]int32)
	var vcnt = 0


	for _, face := range interObj.Faces {
		for _, item := range face.s {
			// var v, t, n int32 = -1, -1, -1
			v, t, n := getVertexTextureNormalIndex(item)
			n -= 1
			t -= 1
			v -= 1
			vertexKey := fmt.Sprintf("%d/%d/%d", v, t, n)
			if idx, ok := vmap[vertexKey]; !ok {
				vmap[vertexKey] = int32(vcnt)
				vcnt += 1

				obj.Vertices = append(obj.Vertices, interObj.Vertices[v].X(), interObj.Vertices[v].Y(), interObj.Vertices[v].Z())
				if t >= 0 {
					obj.Uvs = append(obj.Uvs, interObj.Uvs[t].X(), interObj.Uvs[t].Y())
				}
				if n >= 0 {
					obj.Normals = append(obj.Normals, interObj.Normals[n].X(), interObj.Normals[n].Y(), interObj.Normals[n].Z())
				}

			} else {
				ni := idx * 3

				if n >= 0 && len(obj.Normals) > int(ni+2) {
					var n1 = mgl32.Vec3{
						obj.Normals[ni],
						obj.Normals[ni+1],
						obj.Normals[ni+2],
					}
					var n2 = mgl32.Vec3{
						interObj.Normals[n].X(),
						interObj.Normals[n].Y(),
						interObj.Normals[n].Z(),
					}

					n2 = n2.Add(n1).Normalize()

					obj.Normals[ni], obj.Normals[ni+1], obj.Normals[ni+2] = n2.X(), n2.Y(), n2.Z()
				}
			}
			obj.VertexIndices = append(obj.VertexIndices, uint16(vmap[vertexKey]))
		}

	}


	return nil
}

func atoi(s string) int32  {
	if len(s) > 0 {
		i, _ := strconv.Atoi(s)
		return int32(i)
	}
	return -1
}

func getVertexTextureNormalIndex(face string) (v, t, n int32) {
	v, t, n  = -1, -1, -1
	vals := strings.Split(face, "/")
	if len(vals) > 0 {
		v = atoi(vals[0])
	}
	if len(vals) > 1 {
		t = atoi(vals[1])
	}
	if len(vals) > 2 {
		n = atoi(vals[2])
	}
	return v, t, n
}