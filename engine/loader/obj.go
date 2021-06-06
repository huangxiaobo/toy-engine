package loader

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strings"

	"github.com/go-gl/mathgl/mgl32"

	"toy/engine"
	"toy/engine/logger"
)

func LoadObj(objFile string, obj *engine.Obj) error {
	file, err := os.Open(objFile)
	if err != nil {
		log.Fatal(err)
	}
	defer file.Close()

	//var vertices []*mgl32.Vec3 // vertices
	//var normals []*mgl32.Vec3  // normals
	//var uvs []*mgl32.Vec2      // uvs

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		tokens := strings.Split(line, " ")
		fmt.Println(line)
		switch tokens[0] {
		case "v":
			var vec3 mgl32.Vec3
			// v x y z
			cnt, err := fmt.Sscanf(line, "v %f %f %f", &vec3[0], &vec3[1], &vec3[2])
			if cnt != 3 || err != nil {
				logger.Fatal("read vert failed, cnt: %s, err: %s", cnt, err)
			}

			obj.Vertices = append(obj.Vertices, vec3[0], vec3[1], vec3[2])
		case "vt":
			var vec2 mgl32.Vec2
			// vt u v [w]
			cnt, err := fmt.Sscanf(line, "vt %f %f", &vec2[0], &vec2[1])
			if cnt != 2 || err != nil {
				logger.Fatal("read tang failed, cnt: %s, err: %s", cnt, err)
			}
			obj.Uvs = append(obj.Uvs, vec2[0], vec2[1])
		case "vn":
			var vec3 mgl32.Vec3
			// vn x y z
			cnt, err := fmt.Sscanf(line, "vn %f %f %f", &vec3[0], &vec3[1], &vec3[2])
			if cnt != 3 || err != nil {
				logger.Fatal("read normal failed, cnt: %s, err: %s", cnt, err)
			}
			obj.Normals = append(obj.Normals, vec3[0], vec3[1], vec3[2])
		case "f":
			var v1, v2, v3 [3]uint16
			// f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3] ...
			cnt, err := fmt.Sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
				&v1[0], &v1[1], &v1[2],
				&v2[0], &v2[1], &v2[2],
				&v3[0], &v3[1], &v3[2],
			)
			if cnt != 9 || err != nil {
				logger.Fatal("read normal failed, cnt: %s, err: %s", cnt, err)
			}
			obj.VertexIndices = append(obj.VertexIndices, v1[0]-1, v2[0]-1, v3[0]-1)
			obj.UvIndices = append(obj.UvIndices, v1[1]-1, v2[1]-1, v3[1]-1)
			obj.NormalIndices = append(obj.NormalIndices, v1[2]-1, v2[2]-1, v3[2]-1)
		}
	}
	return nil
}
