package material

import "github.com/go-gl/mathgl/mgl32"

type Material struct {
	AmbientColor  mgl32.Vec3 `ui:"ColorEdit3"`  // 环境
	DiffuseColor  mgl32.Vec3 `ui:"ColorEdit3"`  // 漫反射
	SpecularColor mgl32.Vec3 `ui:"ColorEdit3"`  // 镜面反射
	Shininess     float32    `ui:"SliderFloat"` // 镜面反射光泽
}
