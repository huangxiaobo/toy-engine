package model

import (
	"errors"
	"fmt"
	"github.com/go-gl/gl/v4.1-core/gl"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/huangxiaobo/toy-engine/engine/config"
	"github.com/huangxiaobo/toy-engine/engine/light"
	"github.com/huangxiaobo/toy-engine/engine/logger"
	"github.com/huangxiaobo/toy-engine/engine/material"
	"github.com/huangxiaobo/toy-engine/engine/mesh"
	"github.com/huangxiaobo/toy-engine/engine/shader"
	"github.com/huangxiaobo/toy-engine/engine/technique"
	"github.com/huangxiaobo/toy-engine/engine/texture"
	"github.com/huangxiaobo/toy-engine/engine/utils"
	"github.com/rishabh-bector/assimp-golang"
	"path/filepath"
	"sync"
)

type Model struct {
	texturesLoaded  map[string]texture.Texture
	wg              sync.WaitGroup
	Meshes          []*mesh.Mesh
	GammaCorrection bool
	BasePath        string
	FileName        string

	Name     string
	Id       string
	Material *material.Material
	effect   *technique.LightingTechnique
	shader   *shader.Shader

	Position   mgl32.Vec3
	Scale      mgl32.Vec3
	Rotate     float32
	geoInvalid bool
	model      mgl32.Mat4

	DrawMode uint32
}

func NewModel(xmlModel config.XmlModel) (Model, error) {
	basePath := filepath.Join(utils.GetCurrentDir(), "resource/model", xmlModel.Name)
	m := Model{
		BasePath:        basePath,
		model:           mgl32.Ident4(),
		Name:            xmlModel.Name,
		Id:              xmlModel.Id,
		FileName:        xmlModel.Mesh.File,
		GammaCorrection: xmlModel.GammaCorrection,
		texturesLoaded:  make(map[string]texture.Texture),
		Position:        xmlModel.Position.XYZ(),
		Scale:           xmlModel.Scale.XYZ(),
		effect:          &technique.LightingTechnique{},
		Material: &material.Material{
			AmbientColor:  xmlModel.Material.AmbientColor.RGB(),
			DiffuseColor:  xmlModel.Material.DiffuseColor.RGB(),
			SpecularColor: xmlModel.Material.SpecularColor.RGB(),
			Shininess:     xmlModel.Material.Shininess,
		},
		shader: &shader.Shader{
			VertFilePath: filepath.Join(basePath, xmlModel.Shader.VertFile),
			FragFilePath: filepath.Join(basePath, xmlModel.Shader.FragFile),
		},
	}

	m.Init()

	return m, nil
}

func (m *Model) Init() {
	if err := m.loadModel(); err != nil {
		panic(err)
	}

	// shader
	if err := m.shader.Init(); err != nil {
		logger.Error(err)
		panic(err)
	}
	m.effect.Init(m.shader)

	m.SetPosition(m.Position)
	m.SetScale(m.Scale)
}

func (m *Model) Dispose() {
	for i := 0; i < len(m.Meshes); i++ {
		m.Meshes[i].Dispose()
	}
}

// Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
func (m *Model) loadModel() error {
	if len(m.FileName) == 0 {
		return nil
	}
	// Read file via ASSIMP
	path := filepath.Join(m.BasePath, m.FileName)
	scene := assimp.ImportFile(path, uint(assimp.Process_Triangulate|assimp.Process_FlipUVs))

	// Check for errors
	if scene.Flags()&assimp.SceneFlags_Incomplete != 0 {
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
		m.Meshes[i].Setup()
	}
}

func (m *Model) processNode(aNode *assimp.Node, aScene *assimp.Scene) {
	// Process each mesh located at the current node
	m.wg.Add(aNode.NumMeshes() + aNode.NumChildren())

	for i := 0; i < aNode.NumMeshes(); i++ {
		// The node object only contains indices to index the actual objects in the scene.
		// The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		go func(index int) {
			defer m.wg.Done()
			mi := aScene.Meshes()[aNode.Meshes()[index]]
			ms := m.processMesh(mi, aScene)
			m.Meshes = append(m.Meshes, ms)
		}(i)

	}

	// After we've processed all the meshes (if any) we then recursively process each of the children nodes
	c := aNode.Children()
	for j := 0; j < len(c); j++ {
		go func(n *assimp.Node, s *assimp.Scene) {
			defer m.wg.Done()
			m.processNode(n, s)
		}(c[j], aScene)
	}
}

func (m *Model) processMesh(aMesh *assimp.Mesh, aScene *assimp.Scene) *mesh.Mesh {
	// Return a mesh object created from the extracted mesh data

	return mesh.NewMesh(
		m.processMeshVertices(aMesh),
		m.processMeshIndices(aMesh),
		m.processMeshTextures(aMesh, aScene))
}

func (m *Model) ProcessAssimpMesh(aMesh *assimp.Mesh) []mesh.Vertex {
	// Walk through each of the mesh's vertices
	var vertices []mesh.Vertex

	positions := aMesh.Vertices()

	normals := aMesh.Normals()
	useNormals := len(normals) > 0

	tex := aMesh.TextureCoords(0)
	useTex := true
	if tex == nil {
		useTex = false
	}

	tangents := aMesh.Tangents()
	useTangents := len(tangents) > 0

	bitangents := aMesh.Bitangents()
	useBitTangents := len(bitangents) > 0

	for i := 0; i < aMesh.NumVertices(); i++ {
		// We declare a placeholder vector since assimp uses its own vector class that
		// doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
		vertex := mesh.Vertex{}

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

func (m *Model) processMeshVertices(aMesh *assimp.Mesh) []mesh.Vertex {
	// Walk through each of the mesh's vertices
	var vertices []mesh.Vertex

	positions := aMesh.Vertices()

	colors := aMesh.Colors(0)
	useColors := len(colors) > 0

	normals := aMesh.Normals()
	useNormals := len(normals) > 0

	tex := aMesh.TextureCoords(0)
	useTex := true
	if tex == nil {
		useTex = false
	}

	tangents := aMesh.Tangents()
	useTangents := len(tangents) > 0

	bitangents := aMesh.Bitangents()
	useBitTangents := len(bitangents) > 0

	for i := 0; i < aMesh.NumVertices(); i++ {
		// We declare a placeholder vector since assimp uses its own vector class that
		// doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
		vertex := mesh.Vertex{}

		// Positions
		vertex.Position = mgl32.Vec3{positions[i].X(), positions[i].Y(), positions[i].Z()}

		if useColors {
			vertex.Color = mgl32.Vec3{colors[i].R(), colors[i].G(), colors[i].B()}
		}

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

func (m *Model) processMeshIndices(aMesh *assimp.Mesh) []uint32 {
	var indices []uint32
	// Now wak through each of the aMesh's faces (a face is a aMesh its triangle) and retrieve the corresponding vertex indices.
	for i := 0; i < aMesh.NumFaces(); i++ {
		face := aMesh.Faces()[i]
		// Retrieve all indices of the face and store them in the indices vector
		indices = append(indices, face.CopyIndices()...)
	}
	return indices
}

func (m *Model) processMeshTextures(aMesh *assimp.Mesh, aScene *assimp.Scene) []texture.Texture {
	var textures []texture.Texture
	// Process materials
	if aMesh.MaterialIndex() >= 0 {
		materialObj := aScene.Materials()[aMesh.MaterialIndex()]

		// We assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
		// Same applies to other texture as the following list summarizes:
		// Diffuse: texture_diffuseN
		// Specular: texture_specularN
		// Normal: texture_normalN

		// 1. Diffuse maps
		diffuseMaps := m.loadMaterialTextures(materialObj, assimp.TextureMapping_Diffuse, "texture_diffuse")
		textures = append(textures, diffuseMaps...)
		// 2. Specular maps
		specularMaps := m.loadMaterialTextures(materialObj, assimp.TextureMapping_Specular, "texture_specular")
		textures = append(textures, specularMaps...)
		// 3. Normal maps
		normalMaps := m.loadMaterialTextures(materialObj, assimp.TextureMapping_Height, "texture_normal")
		textures = append(textures, normalMaps...)
		// 4. Height maps
		heightMaps := m.loadMaterialTextures(materialObj, assimp.TextureMapping_Ambient, "texture_height")
		textures = append(textures, heightMaps...)
	}
	return textures
}

func (m *Model) loadMaterialTextures(aMaterial *assimp.Material, aTextureMapping assimp.TextureMapping, tt string) []texture.Texture {
	textureType := assimp.TextureType(aTextureMapping)
	textureCount := aMaterial.GetMaterialTextureCount(textureType)
	var result []texture.Texture

	for i := 0; i < textureCount; i++ {
		file, _, _, _, _, _, _, _ := aMaterial.GetMaterialTexture(textureType, 0)
		filename := m.BasePath + file
		textureObj := texture.Texture{Id: 0, TextureType: tt, Path: filename}
		result = append(result, textureObj)

		//if val, ok := m.texturesLoaded[filename]; ok {
		//	result = append(result, val)
		//} else {
		//	texId := m.textureFromFile(filename)
		//	textureObj := Texture{id: texId, TextureType: tt, Path: filename}
		//	result = append(result, textureObj)
		//	m.texturesLoaded[filename] = textureObj
		//}
	}
	return result
}

func (m *Model) SetScale(scale mgl32.Vec3) {
	m.Scale = scale
	m.geoInvalid = true
}

func (m *Model) SetPosition(p mgl32.Vec3) {
	m.Position = p
	m.geoInvalid = true
}

func (m *Model) SetRotate(rotate float32) {
	m.Rotate = rotate
	m.geoInvalid = true
}

func (m *Model) Update(elapsed float64) {
	if m.geoInvalid {
		m.model = mgl32.Translate3D(m.Position[0], m.Position[1], m.Position[2])
		m.model = m.model.Mul4(mgl32.HomogRotate3D(m.Rotate, mgl32.Vec3{0, 1, 0}))
		m.model = m.model.Mul4(mgl32.Scale3D(m.Scale[0], m.Scale[1], m.Scale[2]))

		m.geoInvalid = false
	}
}

func (m *Model) PreRender() {
	gl.PolygonMode(gl.FRONT, gl.LINE)
}

func (m *Model) Render(projection, model, view mgl32.Mat4, eyePosition *mgl32.Vec3, lights []*light.PointLight) {
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

	m.effect.SetPointLight(lights)
	m.effect.SetMaterial(m.Material)

	gl.BindFragDataLocation(m.effect.ShaderObj.Program, 0, gl.Str("color\x00"))

	for _, mi := range m.Meshes {
		mi.Draw(m.effect.ShaderObj.Program)
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
	Render(projection, model, view mgl32.Mat4, eyePosition *mgl32.Vec3, light []*light.PointLight)
	Update(elapsed float64)
	PreRender()
	PostRender()
}
