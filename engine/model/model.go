package model

import (
	"encoding/xml"
	"errors"
	"fmt"
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/light"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/huangxiaobo/toy-engine/engine/material"
	"github.com/huangxiaobo/toy-engine/engine/shader"
	"github.com/huangxiaobo/toy-engine/engine/technique"
	"github.com/huangxiaobo/toy-engine/engine/texture"
	assimp "github.com/rishabh-bector/assimp-golang"
	"io/ioutil"
	"path/filepath"
	"sync"
)

type Model struct {
	texturesLoaded  map[string]texture.Texture
	wg              sync.WaitGroup
	Meshes          []Mesh
	GammaCorrection bool
	BasePath        string
	FileName        string

	Name     string
	Id       string
	material *material.Material
	effect   *technique.LightingTechnique

	Position mgl32.Vec3
	Scale    mgl32.Vec3
	model    mgl32.Mat4

	DrawMode uint32
}

type XmlModel struct {
	XMLName         xml.Name    `xml:"model"`
	XMLAlias        string      `xml:"name"`
	XMLId           string      `xml:"id"`
	XMLPostion      XmlPostion  `xml:"postion"`
	XMLScale        XmlScale    `xml:"scale"`
	XMLMesh         XmlMesh     `xml:"mesh"`
	XmlShader       XmlShader   `xml:"shader"`
	GammaCorrection bool        `xml:"gammacorrection"`
	XMLMaterial     XmlMaterial `xml:"material"`
}

type XmlMesh struct {
	File string `xml:"file"` // Mesh file
}
type XmlShader struct {
	VertFile string `xml:"vert"`
	FragFile string `xml:"frag"`
}

type XmlMaterial struct {
	Ambientcolor  XmlColor `xml:"ambient"`
	Diffusecolor  XmlColor `xml:"diffuse"`
	Specularcolor XmlColor `xml:"specular"`
	Shininess     float32  `xml:"shininess"`
}

type XmlColor struct {
	R float32 `xml:"r"`
	G float32 `xml:"g"`
	B float32 `xml:"b"`
}

type XmlPostion struct {
	X float32 `xml:"x"`
	Y float32 `xml:"y"`
	Z float32 `xml:"z"`
}

type XmlScale struct {
	X float32 `xml:"x"`
	Y float32 `xml:"y"`
	Z float32 `xml:"z"`
}

func NewModel(f string) (Model, error) {
	f, _ = filepath.Abs(f)

	m := Model{
		BasePath: filepath.Dir(f),
		model:    mgl32.Ident4(),
	}

	xm := m.loadXml(f)

	m.Name = xm.XMLAlias
	m.Id = xm.XMLId

	m.texturesLoaded = make(map[string]texture.Texture)
	if err := m.loadModel(); err != nil {
		panic(err)
	}

	m.effect = &technique.LightingTechnique{}

	// Material
	m.material = &material.Material{}
	m.material.AmbientColor = mgl32.Vec3{xm.XMLMaterial.Ambientcolor.R, xm.XMLMaterial.Ambientcolor.G, xm.XMLMaterial.Ambientcolor.B}
	m.material.DiffuseColor = mgl32.Vec3{xm.XMLMaterial.Diffusecolor.R, xm.XMLMaterial.Diffusecolor.G, xm.XMLMaterial.Diffusecolor.B}
	m.material.SpecularColor = mgl32.Vec3{xm.XMLMaterial.Specularcolor.R, xm.XMLMaterial.Specularcolor.G, xm.XMLMaterial.Specularcolor.B}
	m.material.Shininess = xm.XMLMaterial.Shininess

	s := &shader.Shader{
		VertFilePath: filepath.Join(m.BasePath, xm.XmlShader.VertFile),
		FragFilePath: filepath.Join(m.BasePath, xm.XmlShader.FragFile),
	}
	if err := s.Init(); err != nil {
		logger.Error(err)
		panic("errr")
	}
	m.effect.Init(s)

	m.Position = mgl32.Vec3{xm.XMLPostion.X, xm.XMLPostion.Y, xm.XMLPostion.Z}
	m.Scale = mgl32.Vec3{xm.XMLScale.X, xm.XMLScale.Y, xm.XMLScale.Z}

	m.SetPostion(m.Position)
	m.SetScale(m.Scale)

	return m, nil
}

func (m *Model) Dispose() {
	for i := 0; i < len(m.Meshes); i++ {
		gl.DeleteVertexArrays(1, &m.Meshes[i].vao)
		gl.DeleteBuffers(1, &m.Meshes[i].vbo)
		gl.DeleteBuffers(1, &m.Meshes[i].ebo)
	}
}

func (m *Model) loadXml(f string) *XmlModel {
	data, err := ioutil.ReadFile(f)
	if err != nil {
		panic(err)
	}

	xm := &XmlModel{}
	err = xml.Unmarshal(data, &xm)
	if err != nil {
		fmt.Printf("error: %xm", err)
		panic(err)
	}

	m.FileName = xm.XMLMesh.File
	m.GammaCorrection = xm.GammaCorrection

	return xm
}

// Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
func (m *Model) loadModel() error {
	// Read file via ASSIMP
	path := m.BasePath + m.FileName
	fmt.Println(path)
	scene := assimp.ImportFile(path, uint(
		assimp.Process_Triangulate|assimp.Process_FlipUVs))

	// Check for errors
	if scene.Flags()&assimp.SceneFlags_Incomplete != 0 { // if is Not Zero
		fmt.Printf("ERROR::ASSIMP:: %v\n", scene.Flags())
		return errors.New("shit failed")
	}

	// Process ASSIMP's root node recursively
	m.processNode(scene.RootNode(), scene)
	m.wg.Wait()
	m.initGL()
	return nil
}

func (m *Model) initGL() {
	// using a for loop with a range doesnt work here?!
	// also making a temp var inside the loop doesnt work either?!
	for i := 0; i < len(m.Meshes); i++ {
		for j := 0; j < len(m.Meshes[i].Textures); j++ {
			if val, ok := m.texturesLoaded[m.Meshes[i].Textures[j].Path]; ok {
				m.Meshes[i].Textures[j].Id = val.Id
			} else {
				m.Meshes[i].Textures[j].Id = m.textureFromFile(m.Meshes[i].Textures[j].Path)
				m.texturesLoaded[m.Meshes[i].Textures[j].Path] = m.Meshes[i].Textures[j]
			}
		}
		m.Meshes[i].setup()
	}
}

func (m *Model) processNode(n *assimp.Node, s *assimp.Scene) {
	// Process each mesh located at the current node
	m.wg.Add(n.NumMeshes() + n.NumChildren())

	for i := 0; i < n.NumMeshes(); i++ {
		// The node object only contains indices to index the actual objects in the scene.
		// The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		go func(index int) {
			defer m.wg.Done()
			mesh := s.Meshes()[n.Meshes()[index]]
			ms := m.processMesh(mesh, s)
			m.Meshes = append(m.Meshes, ms)
		}(i)

	}

	// After we've processed all the meshes (if any) we then recursively process each of the children nodes
	c := n.Children()
	for j := 0; j < len(c); j++ {
		go func(n *assimp.Node, s *assimp.Scene) {
			defer m.wg.Done()
			m.processNode(n, s)
		}(c[j], s)
	}
}

func (m *Model) processMesh(ms *assimp.Mesh, s *assimp.Scene) Mesh {
	// Return a mesh object created from the extracted mesh data

	return NewMesh(
		m.processMeshVertices(ms),
		m.processMeshIndices(ms),
		m.processMeshTextures(ms, s))
}

func (m *Model) ProcessAssimpMesh(mesh *assimp.Mesh) []Vertex {
	// Walk through each of the mesh's vertices
	var vertices []Vertex

	positions := mesh.Vertices()

	normals := mesh.Normals()
	useNormals := len(normals) > 0

	tex := mesh.TextureCoords(0)
	useTex := true
	if tex == nil {
		useTex = false
	}

	tangents := mesh.Tangents()
	useTangents := len(tangents) > 0

	bitangents := mesh.Bitangents()
	useBitTangents := len(bitangents) > 0

	for i := 0; i < mesh.NumVertices(); i++ {
		// We declare a placeholder vector since assimp uses its own vector class that
		// doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
		vertex := Vertex{}

		// Positions
		vertex.Position = mgl32.Vec3{positions[i].X(), positions[i].Y(), positions[i].Z()}

		// Normals
		if useNormals {
			vertex.Normal = mgl32.Vec3{normals[i].X(), normals[i].Y(), normals[i].Z()}
			//n.WriteString(fmt.Sprintf("[%f, %f, %f]\n", tmp[i].X(), tmp[i].Y(), tmp[i].Z()))
		}

		// Texture Coordinates
		if useTex {
			// Does the mesh contain texture coordinates?
			// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			vertex.TexCoords = mgl32.Vec2{tex[i].X(), tex[i].Y()}
		} else {
			vertex.TexCoords = mgl32.Vec2{0.0, 0.0}
		}

		// Tangent
		if useTangents {
			vertex.Tangent = mgl32.Vec3{tangents[i].X(), tangents[i].Y(), tangents[i].Z()}
		}

		// Bitangent
		if useBitTangents {
			vertex.Bitangent = mgl32.Vec3{bitangents[i].X(), bitangents[i].Y(), bitangents[i].Z()}
		}

		vertices = append(vertices, vertex)
	}

	return vertices
}

func (m *Model) processMeshVertices(mesh *assimp.Mesh) []Vertex {
	// Walk through each of the mesh's vertices
	var vertices []Vertex

	positions := mesh.Vertices()

	normals := mesh.Normals()
	useNormals := len(normals) > 0

	tex := mesh.TextureCoords(0)
	useTex := true
	if tex == nil {
		useTex = false
	}

	tangents := mesh.Tangents()
	useTangents := len(tangents) > 0

	bitangents := mesh.Bitangents()
	useBitTangents := len(bitangents) > 0

	for i := 0; i < mesh.NumVertices(); i++ {
		// We declare a placeholder vector since assimp uses its own vector class that
		// doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
		vertex := Vertex{}

		// Positions
		vertex.Position = mgl32.Vec3{positions[i].X(), positions[i].Y(), positions[i].Z()}

		// Normals
		if useNormals {
			vertex.Normal = mgl32.Vec3{normals[i].X(), normals[i].Y(), normals[i].Z()}
			//n.WriteString(fmt.Sprintf("[%f, %f, %f]\n", tmp[i].X(), tmp[i].Y(), tmp[i].Z()))
		}

		// Texture Coordinates
		if useTex {
			// Does the mesh contain texture coordinates?
			// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			vertex.TexCoords = mgl32.Vec2{tex[i].X(), tex[i].Y()}
		} else {
			vertex.TexCoords = mgl32.Vec2{0.0, 0.0}
		}

		// Tangent
		if useTangents {
			vertex.Tangent = mgl32.Vec3{tangents[i].X(), tangents[i].Y(), tangents[i].Z()}
		}

		// Bitangent
		if useBitTangents {
			vertex.Bitangent = mgl32.Vec3{bitangents[i].X(), bitangents[i].Y(), bitangents[i].Z()}
		}

		vertices = append(vertices, vertex)
	}

	return vertices
}

func (m *Model) processMeshIndices(mesh *assimp.Mesh) []uint32 {
	var indices []uint32
	// Now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for i := 0; i < mesh.NumFaces(); i++ {
		face := mesh.Faces()[i]
		// Retrieve all indices of the face and store them in the indices vector
		indices = append(indices, face.CopyIndices()...)
	}
	return indices
}

func (m *Model) processMeshTextures(mesh *assimp.Mesh, s *assimp.Scene) []texture.Texture {
	var textures []texture.Texture
	// Process materials
	if mesh.MaterialIndex() >= 0 {
		material := s.Materials()[mesh.MaterialIndex()]

		// We assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
		// Same applies to other texture as the following list summarizes:
		// Diffuse: texture_diffuseN
		// Specular: texture_specularN
		// Normal: texture_normalN

		// 1. Diffuse maps
		diffuseMaps := m.loadMaterialTextures(material, assimp.TextureMapping_Diffuse, "texture_diffuse")
		textures = append(textures, diffuseMaps...)
		// 2. Specular maps
		specularMaps := m.loadMaterialTextures(material, assimp.TextureMapping_Specular, "texture_specular")
		textures = append(textures, specularMaps...)
		// 3. Normal maps
		normalMaps := m.loadMaterialTextures(material, assimp.TextureMapping_Height, "texture_normal")
		textures = append(textures, normalMaps...)
		// 4. Height maps
		heightMaps := m.loadMaterialTextures(material, assimp.TextureMapping_Ambient, "texture_height")
		textures = append(textures, heightMaps...)
	}
	return textures
}

func (m *Model) loadMaterialTextures(ms *assimp.Material, tm assimp.TextureMapping, tt string) []texture.Texture {
	textureType := assimp.TextureType(tm)
	textureCount := ms.GetMaterialTextureCount(textureType)
	var result []texture.Texture

	for i := 0; i < textureCount; i++ {
		file, _, _, _, _, _, _, _ := ms.GetMaterialTexture(textureType, 0)
		filename := m.BasePath + file
		texture := texture.Texture{Id: 0, TextureType: tt, Path: filename}
		result = append(result, texture)

		//if val, ok := m.texturesLoaded[filename]; ok {
		//	result = append(result, val)
		//} else {
		//	texId := m.textureFromFile(filename)
		//	texture := Texture{id: texId, TextureType: tt, Path: filename}
		//	result = append(result, texture)
		//	m.texturesLoaded[filename] = texture
		//}
	}
	return result
}

func (m *Model) SetScale(scale mgl32.Vec3) {
	m.Scale = scale
	m.model = m.model.Mul4(mgl32.Scale3D(m.Scale[0], m.Scale[1], m.Scale[2]))
}

func (m *Model) SetPostion(p mgl32.Vec3) {
	m.model = m.model.Add(mgl32.Translate3D(p[0], p[1], p[2]))

}

func (m *Model) Update(elapsed float64) {
	m.model = m.model.Mul4(mgl32.HomogRotate3DY(float32(elapsed)))
}

func (m *Model) PreRender() {
	gl.PolygonMode(gl.FRONT, gl.LINE)
}

func (m *Model) Render(projection, model, view mgl32.Mat4, eyePosition *mgl32.Vec3, light *light.PointLight) {
	// RenderObj
	model = model.Mul4(m.model)
	mvp := projection.Mul4(view).Mul4(model)

	// Effect
	m.effect.Enable()
	m.effect.SetProjectMatrix(&projection)
	m.effect.SetViewMatrix(&view)
	m.effect.SetModelMatrix(&model)
	m.effect.SetWVP(&mvp)
	m.effect.SetEyeWorldPos(eyePosition)

	m.effect.SetPointLight(light)
	m.effect.SetMaterial(m.material)

	gl.BindFragDataLocation(m.effect.ShaderObj.Program, 0, gl.Str("color\x00"))

	for _, mesh := range m.Meshes {
		mesh.draw(m.effect.ShaderObj.Program)
	}
	m.effect.Disable()
}

func (m *Model) PostRender() {
	gl.PolygonMode(gl.FRONT, gl.LINE)
}

func (m *Model) textureFromFile(f string) uint32 {
	//Generate texture ID and load texture data
	if tex, err := texture.NewTexture(gl.REPEAT, gl.REPEAT, gl.LINEAR_MIPMAP_LINEAR, gl.LINEAR, f); err != nil {
		panic(err)
	} else {
		return tex
	}
}

// RenderObj 可渲染對象
type RenderObj interface {
	Render(projection, model, view mgl32.Mat4, eyePosition *mgl32.Vec3, light *light.PointLight)
	Update(elapsed float64)
	PreRender()
	PostRender()
}
