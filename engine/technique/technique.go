package technique

import (
	"fmt"

	"github.com/go-gl/gl/v4.1-core/gl"

	"github.com/huangxiaobo/toy-engine/engine/shader"
)

type Technique struct {
	ShaderObj *shader.Shader
}

func (t *Technique) Init(shader *shader.Shader) {
	t.ShaderObj = shader
}

func (t *Technique) Finalize() {

}

func (t *Technique) Enable() bool {
	t.ShaderObj.Use()
	return true
}

func (t *Technique) Disable() bool {
	t.ShaderObj.UnUse()
	return true
}

func (t *Technique) GetUniformLocation(pUniformName string) int32 {
	Location := gl.GetUniformLocation(t.ShaderObj.Program, gl.Str(fmt.Sprintf("%s\x00", pUniformName)))

	if Location == -1 {
		fmt.Printf("Warning! Unable to get the location of uniform '%s'\n", pUniformName)
	}

	return Location
}

func (t *Technique) GetProgramParam(param uint32) int32 {
	var ret int32 = -1
	gl.GetProgramiv(t.ShaderObj.Program, param, &ret)
	return ret
}
