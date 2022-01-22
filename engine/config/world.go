package config

import (
	"encoding/xml"
	"fmt"
	"github.com/go-gl/mathgl/mgl32"
	"io/ioutil"
)

type XmlRGB struct {
	R float32 `xml:"r"`
	G float32 `xml:"g"`
	B float32 `xml:"b"`
	A float32 `xml:"a"`
}

func (rgb *XmlRGB) RGB() mgl32.Vec3 {
	return mgl32.Vec3{rgb.R, rgb.G, rgb.B}
}

func (rgb *XmlRGB) RGBA() mgl32.Vec4 {
	return mgl32.Vec4{rgb.R, rgb.G, rgb.B, rgb.A}
}

type XmlXYZ struct {
	X float32 `xml:"x"`
	Y float32 `xml:"y"`
	Z float32 `xml:"z"`
	W float32 `xml:"w"`
}

func (xyz *XmlXYZ) XYZ() mgl32.Vec3 {
	return mgl32.Vec3{xyz.Z, xyz.Y, xyz.Z}
}

func (xyz *XmlXYZ) XYZW() mgl32.Vec4 {
	return mgl32.Vec4{xyz.Z, xyz.Y, xyz.Z, xyz.W}
}

type XmlTarget = XmlXYZ

type XmlCamera struct {
	XMLPosition XmlXYZ `xml:"position"`
	XMLTarget   XmlXYZ `xml:"target"`
}

type XmlLight struct {
	XMLPosition XmlXYZ `xml:"position"`
	XMLColor    XmlRGB `xml:"color"`
}

type XmlMesh struct {
	File string `xml:"file"` // Mesh file
}
type XmlShader struct {
	VertFile string `xml:"vert"`
	FragFile string `xml:"frag"`
}

type XmlMaterial struct {
	AmbientColor  XmlRGB  `xml:"ambient"`
	DiffuseColor  XmlRGB  `xml:"diffuse"`
	SpecularColor XmlRGB  `xml:"specular"`
	Shininess     float32 `xml:"shininess"`
}

type XmlModel struct {
	XmlResourceClass string `xml:"resource_class,attr"`

	Name            string      `xml:"name"`
	Id              string      `xml:"id"`
	Position        XmlXYZ      `xml:"position"`
	Scale           XmlXYZ      `xml:"scale"`
	Mesh            XmlMesh     `xml:"mesh"`
	Shader          XmlShader   `xml:"shader"`
	GammaCorrection bool        `xml:"gammacorrection"`
	Material        XmlMaterial `xml:"material"`
}

type XmlModels struct {
	XMLName   xml.Name   `xml:"models"`
	XMLModels []XmlModel `xml:"model"`
}

type XmlWindow struct {
	XMLName   xml.Name `xml:"window"`
	XMLWidth  int32    `xml:"width"`
	XMLHeight int32    `xml:"height"`
}

type XmlWorld struct {
	XMLName   xml.Name  `xml:"world"`
	XMLWindow XmlWindow `xml:"window"`
	XMLCamera XmlCamera `xml:"camera"`
	XMLLight  XmlLight  `xml:"light"`
	XMLModels XmlModels `xml:"models"`
}

func InitXML(file string) *XmlWorld {

	data, err := ioutil.ReadFile("./resource/world.xml")
	if err != nil {
		panic(err)
	}

	xmlWorld := &XmlWorld{}
	err = xml.Unmarshal(data, &xmlWorld)
	if err != nil {
		fmt.Printf("error: %xm", err)
		panic(err)
	}

	Config.WindowWidth = xmlWorld.XMLWindow.XMLWidth
	Config.WindowHeight = xmlWorld.XMLWindow.XMLHeight

	return xmlWorld
}
