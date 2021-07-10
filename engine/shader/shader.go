package shader

import (
	"fmt"
	"io/ioutil"
	"reflect"
	"strings"

	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"

	"toy/engine/logger"
)

type Shader struct {
	VertFilePath string
	FragFilePath string
	Program      uint32
}

func (s *Shader) Init() error {
	vsData, err := ioutil.ReadFile(s.VertFilePath)
	if err != nil {
		fmt.Println(err)
	}
	fsData, err := ioutil.ReadFile(s.FragFilePath)
	if err != nil {
		fmt.Println(err)
	}

	s.Program, err = s.NewProgram(string(vsData)+"\x00", string(fsData)+"\x00")
	if err != nil {
		panic(err)
	}
	return nil
}

func (s *Shader) NewProgram(vertexShaderSource, fragmentShaderSource string) (uint32, error) {
	// 加载并编译shader
	vertexShader, err := s.CompileShader(vertexShaderSource, gl.VERTEX_SHADER)
	if err != nil {
		return 0, err
	}

	fragmentShader, err := s.CompileShader(fragmentShaderSource, gl.FRAGMENT_SHADER)
	if err != nil {
		return 0, err
	}

	// program
	program := gl.CreateProgram()
	gl.AttachShader(program, vertexShader)
	gl.AttachShader(program, fragmentShader)
	gl.LinkProgram(program)

	var status int32
	gl.GetProgramiv(program, gl.LINK_STATUS, &status)
	if status == gl.FALSE {
		var logLength int32
		gl.GetProgramiv(program, gl.INFO_LOG_LENGTH, &logLength)

		log := strings.Repeat("\x00", int(logLength+1))
		gl.GetProgramInfoLog(program, logLength, nil, gl.Str(log))

		return 0, fmt.Errorf("failed to link program: %v", log)
	}

	gl.DeleteShader(vertexShader)
	gl.DeleteShader(fragmentShader)

	return program, nil
}

func (s *Shader) CompileShader(source string, shaderType uint32) (uint32, error) {
	fmt.Println(source)
	shader := gl.CreateShader(shaderType)

	csource, free := gl.Strs(source)
	gl.ShaderSource(shader, 1, csource, nil)
	free()
	gl.CompileShader(shader)

	var status int32
	gl.GetShaderiv(shader, gl.COMPILE_STATUS, &status)
	if status == gl.FALSE {
		var logLength int32
		gl.GetShaderiv(shader, gl.INFO_LOG_LENGTH, &logLength)

		log := strings.Repeat("\x00", int(logLength+1))
		gl.GetShaderInfoLog(shader, logLength, nil, gl.Str(log))

		return 0, fmt.Errorf("failed to compile %v: %v", source, log)
	}

	return shader, nil
}

func (s *Shader) Use() uint32 {
	gl.UseProgram(s.Program)
	return s.Program
}

func (s *Shader) SetUniform(name string, value interface{}) {
	loc := gl.GetUniformLocation(s.Program, gl.Str(name+"\x00"))
	if loc < 0 {
		return
	}

	getType := reflect.TypeOf(value)
	getValue := reflect.ValueOf(value)

	switch getType.Name() {
	case "int":
		gl.Uniform1i(loc, int32(getValue.Int()))
	case "Vec3":
		v := getValue.Interface().(mgl32.Vec3)
		gl.Uniform3fv(loc, 1, &v[0])
	case "Vec4":
		v := getValue.Interface().(mgl32.Vec4)
		gl.Uniform4fv(loc, 1, &v[0])
	case "Mat4":
		v := getValue.Interface().(mgl32.Mat4)
		gl.UniformMatrix4fv(loc, 1, false, &v[0])

	default:
		logger.Info("Un-support type ", getType)
	}

}
