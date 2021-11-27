package technique

import (
	"fmt"

	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"

	"github/huangxiaobo/ToyEngine/engine/shader"
)

type BaseTechnique struct {
	Technique

	projectionUniform int32
	viewUniform       int32
	modelUniform      int32
	wvpUniform        int32
	cameraUniform     int32
}

func (t *BaseTechnique) Init(shader *shader.Shader) {
	t.Technique.Init(shader)

	// Shader
	t.projectionUniform = t.GetUniformLocation("projection")
	t.viewUniform = t.GetUniformLocation("view")
	t.modelUniform = t.GetUniformLocation("model")
	t.wvpUniform = t.GetUniformLocation("gWVP")
	t.cameraUniform = t.GetUniformLocation("gViewPos")
	fmt.Printf("project: %d view: %d, view: %d wvp: %d\n", t.projectionUniform, t.viewUniform, t.modelUniform, t.wvpUniform)
}

// 设置模型-视图矩阵
func (t *BaseTechnique) SetWVP(WVP *mgl32.Mat4) {
	gl.UniformMatrix4fv(t.wvpUniform, 1, false, &((*WVP)[0]))
}

// 设置世界坐标系变换矩阵
func (t *BaseTechnique) SetWorldMatrix(WorldInverse *mgl32.Mat4) {

}

// 设置世界坐标系变换矩阵
func (t *BaseTechnique) SetProjectMatrix(ProjectMatrix *mgl32.Mat4) {
	gl.UniformMatrix4fv(t.projectionUniform, 1, false, &((*ProjectMatrix)[0]))
}

func (t *BaseTechnique) SetViewMatrix(ViewMatrix *mgl32.Mat4) {
	gl.UniformMatrix4fv(t.viewUniform, 1, false, &((*ViewMatrix)[0]))
}

func (t *BaseTechnique) SetModelMatrix(ModelMatrix *mgl32.Mat4) {
	gl.UniformMatrix4fv(t.modelUniform, 1, false, &((*ModelMatrix)[0]))
}

// 设置纹理单元
func (t *BaseTechnique) SetTextureUnit(TextureUnit uint32) {

}

func (t *BaseTechnique) SetEyeWorldPos(EyeWorldPos *mgl32.Vec3) {
	gl.Uniform3f(t.cameraUniform, EyeWorldPos.X(), EyeWorldPos.Y(), EyeWorldPos.Z())
}
