package technique

import (
	"fmt"

	"github.com/go-gl/gl/v4.1-core/gl"

	"github/huangxiaobo/ToyEngine/engine/light"
	"github/huangxiaobo/ToyEngine/engine/material"
	"github/huangxiaobo/ToyEngine/engine/shader"
)

type LightUniform struct {
	Color    int32
	Position int32

	AmbientIntensity int32
	DiffuseIntensity int32
	DiffuseColor     int32
	SpecularColor    int32
	Atten            struct {
		Constant int32
		Linear   int32
		Exp      int32
	}
}

type MaterialUniform struct {
	AmbientColor  int32 // 环境
	DiffuseColor  int32 // 漫反射
	SpecularColor int32 // 镜面反射
	Shininess     int32 // 镜面反射光泽
}

type LightingTechnique struct {
	BaseTechnique

	projectionUniform int32
	viewUniform       int32
	modelUniform      int32
	wvpUniform        int32

	lightUniform [8]LightUniform

	materialUniform MaterialUniform
}

func (t *LightingTechnique) Init(shader *shader.Shader) {
	t.BaseTechnique.Init(shader)

	// Shader
	t.projectionUniform = t.GetUniformLocation("projection")
	t.viewUniform = t.GetUniformLocation("view")
	t.modelUniform = t.GetUniformLocation("model")
	t.wvpUniform = t.GetUniformLocation("gWVP")

	var name string
	for i := 0; i < 1; i++ {
		name = fmt.Sprintf("gLight[%d].Color", i)
		t.lightUniform[i].Color = t.GetUniformLocation(name)

		name = fmt.Sprintf("gLight[%d].Position", i)
		t.lightUniform[i].Position = t.GetUniformLocation(name)

		name = fmt.Sprintf("gLight[%d].AmbientIntensity", i)
		t.lightUniform[i].AmbientIntensity = t.GetUniformLocation(name)

		name = fmt.Sprintf("gLight[%d].DiffuseIntensity", i)
		t.lightUniform[i].DiffuseIntensity = t.GetUniformLocation(name)

		name = fmt.Sprintf("gLight[%d].DiffuseColor", i)
		t.lightUniform[i].DiffuseColor = t.GetUniformLocation(name)

		name = fmt.Sprintf("gLight[%d].SpecularColor", i)
		t.lightUniform[i].SpecularColor = t.GetUniformLocation(name)

		name = fmt.Sprintf("gLight[%d].Atten.Constant", i)
		t.lightUniform[i].Atten.Constant = t.GetUniformLocation(name)

		name = fmt.Sprintf("gLight[%d].Atten.Linear", i)
		t.lightUniform[i].Atten.Linear = t.GetUniformLocation(name)

		name = fmt.Sprintf("gLight[%d].Atten.Exp", i)
		t.lightUniform[i].Atten.Exp = t.GetUniformLocation(name)
	}

	name = "gMaterial.AmbientColor"
	t.materialUniform.AmbientColor = t.GetUniformLocation(name)
	name = "gMaterial.DiffuseColor"
	t.materialUniform.DiffuseColor = t.GetUniformLocation(name)
	name = "gMaterial.SpecularColor"
	t.materialUniform.SpecularColor = t.GetUniformLocation(name)
	name = "gMaterial.Shininess"
	t.materialUniform.Shininess = t.GetUniformLocation(name)
}

func (t *LightingTechnique) SetPointLight(light *light.PointLight) {
	for i := 0; i < 1; i++ {
		gl.Uniform3f(t.lightUniform[i].Color, light.Color.X(), light.Color.Y(), light.Color.Z())
		gl.Uniform1f(t.lightUniform[i].AmbientIntensity, light.AmbientIntensity)
		gl.Uniform1f(t.lightUniform[i].DiffuseIntensity, light.DiffuseIntensity)
		gl.Uniform3f(t.lightUniform[i].Position, light.Position.X(), light.Position.Y(), light.Position.Z())
		gl.Uniform1f(t.lightUniform[i].Atten.Constant, light.Atten.Constant)
		gl.Uniform1f(t.lightUniform[i].Atten.Linear, light.Atten.Linear)
		gl.Uniform1f(t.lightUniform[i].Atten.Exp, light.Atten.Exp)
	}
}

func (t *LightingTechnique) SetMaterial(m *material.Material) {
	gl.Uniform3f(t.materialUniform.AmbientColor, m.AmbientColor.X(), m.AmbientColor.Y(), m.AmbientColor.Z())
	gl.Uniform3f(t.materialUniform.DiffuseColor, m.DiffuseColor.X(), m.DiffuseColor.Y(), m.DiffuseColor.Z())
	gl.Uniform3f(t.materialUniform.SpecularColor, m.SpecularColor.X(), m.SpecularColor.Y(), m.SpecularColor.Z())
	gl.Uniform1f(t.materialUniform.Shininess, m.Shininess)
}
