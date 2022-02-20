package light

type LightType string

const (
	LightTypePoint     LightType = "PointLight"
	LightTypeDirection LightType = "DirectionLight"
	LightTypeSpot      LightType = "SpotLight"
)
