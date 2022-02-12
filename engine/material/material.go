package material

import "github.com/go-gl/mathgl/mgl32"

type Material struct {
	AmbientColor  mgl32.Vec3 // 环境
	DiffuseColor  mgl32.Vec3 // 漫反射
	SpecularColor mgl32.Vec3 // 镜面反射
	Shininess     float32    // 镜面反射光泽
}
