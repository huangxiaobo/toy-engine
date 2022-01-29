package ui

import (
	"fmt"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/inkyblackness/imgui-go/v4"
	"reflect"
)

type WindowMaterialItem struct {
	tag string
	key reflect.StructField
	val reflect.Value
}

type WindowModel struct {
	noClose bool
	visible bool
	flags   WindowFlags

	modelObj interface{}
	content  string

	showDemoWindow bool
}

func NewWindowMaterial() *WindowModel {
	w := &WindowModel{
		visible:  false,
		flags:    WindowFlags{noResize: true, noMenu: true, noTitlebar: true, noCollapse: true, noBackground: false},
		modelObj: nil,
	}

	return w
}

func (w *WindowModel) Reset() {
	w.modelObj = nil
}

//var colorUI = [3]float32{-1, -1, -1}

func (w *WindowModel) Show(displaySize [2]float32) {
	if !w.visible || w.modelObj == nil {
		return
	}
	imgui.SetNextWindowPosV(imgui.Vec2{X: displaySize[0] - 320, Y: 0}, imgui.ConditionNone, imgui.Vec2{})
	imgui.SetNextWindowSizeV(imgui.Vec2{X: 320, Y: displaySize[1]}, imgui.ConditionNone)

	if !imgui.BeginV("ModelPanel", &w.visible, w.flags.combined()) {
		imgui.End()
		return
	}

	imgui.PushItemWidth(imgui.FontSize() * -12)

	rPtrType := reflect.TypeOf(w.modelObj)
	rPtrVal := reflect.ValueOf(w.modelObj)

	rType := rPtrType
	rVal := rPtrVal
	if rPtrType.Kind() == reflect.Ptr {
		rType = rPtrType.Elem()
		rVal = rPtrVal.Elem()
	}

	imgui.Spacing()
	imgui.Bullet()
	imgui.Text("Name")
	imgui.Indent()
	name := rVal.FieldByName("Name").String()
	id := rVal.FieldByName("Id").String()
	imgui.Text(fmt.Sprintf("%v(%v)", name, id))

	imgui.Unindent()
	imgui.Spacing()
	imgui.Spacing()
	imgui.Bullet()
	imgui.Text("Geometry")
	imgui.Indent()

	w.ShowFloat3(rPtrType, rPtrVal, "Position")
	w.ShowFloat3(rPtrType, rPtrVal, "Scale")
	w.ShowFloat(rPtrType, rPtrVal, "Rotate")

	imgui.Unindent()
	imgui.Spacing()
	imgui.Spacing()
	imgui.Bullet()
	imgui.Text("Material")
	imgui.Indent()

	material := rVal.FieldByName("Material").Interface()
	rType = reflect.TypeOf(material)
	rVal = reflect.ValueOf(material)
	if rType.Kind() == reflect.Ptr {
		rType = rType.Elem()
		rVal = rVal.Elem()
	}
	for i := 0; i < rType.NumField(); i++ {
		tag := rType.Field(i).Tag.Get("ui")
		if len(tag) == 0 {
			continue
		}

		key := rType.Field(i)
		val := rVal.Field(i)

		if tag == "ColorEdit3" {
			color := val.Interface().(mgl32.Vec3)
			var colorUI = [3]float32{-1, -1, -1}
			colorUI[0] = color.X()
			colorUI[1] = color.Y()
			colorUI[2] = color.Z()
			imgui.ColorEdit3(key.Name, &colorUI)
			color = val.Interface().(mgl32.Vec3)
			if colorUI[0] < 0 {
				colorUI[0] = color.X()
				colorUI[1] = color.Y()
				colorUI[2] = color.Z()
			} else {
				if val.CanAddr() {
					val.Set(reflect.ValueOf(mgl32.Vec3{colorUI[0], colorUI[1], colorUI[2]}))
				}
			}
		} else if tag == "SliderFloat" {
			value := val.Interface().(float32)
			imgui.SliderFloat(key.Name, &value, 0, 100)
			if val.CanAddr() {
				val.SetFloat(float64(value))
			}
		}
	}
	imgui.Unindent()

	// End of ShowDemoWindow()
	imgui.End()

	if w.showDemoWindow {
		// Normally user code doesn't need/want to call this because positions are saved in .ini file anyway.
		// Here we just want to make the demo initial state a bit more friendly!
		const demoX = 650
		const demoY = 20
		imgui.SetNextWindowPosV(imgui.Vec2{X: demoX, Y: demoY}, imgui.ConditionFirstUseEver, imgui.Vec2{})

		imgui.ShowDemoWindow(&w.showDemoWindow)
	}
}

func (w *WindowModel) ShowFloat3(rPtrType reflect.Type, rPtrVal reflect.Value, fieldName string) {
	position := rPtrVal.Elem().FieldByName(fieldName)
	f3 := position.Interface().(mgl32.Vec3)

	imgui.DragFloat3(fieldName, (*[3]float32)(&f3))

	methodName := fmt.Sprintf("Set%s", fieldName)
	method, err := rPtrType.MethodByName(methodName)
	if err != true {
		fmt.Printf("%v %v: %v\n", rPtrType, methodName, err)
		return
	}
	//fmt.Printf("method: %v, %v\n", method, colorUI)
	method.Func.Call([]reflect.Value{rPtrVal, reflect.ValueOf(f3)})
}

func (w *WindowModel) ShowFloat(rPtrType reflect.Type, rPtrVal reflect.Value, fieldName string) {
	position := rPtrVal.Elem().FieldByName(fieldName)
	f1 := position.Interface().(float32)

	imgui.DragFloat(fieldName, &f1)

	methodName := fmt.Sprintf("Set%s", fieldName)
	method, err := rPtrType.MethodByName(methodName)
	if err != true {
		fmt.Printf("%v %v: %v\n", rPtrType, methodName, err)
		return
	}
	method.Func.Call([]reflect.Value{rPtrVal, reflect.ValueOf(f1)})
}

func (w *WindowModel) SetRenderObj(renderObj interface{}) {
	w.modelObj = renderObj
}

func (w *WindowModel) SetVisible(visible bool) {
	w.visible = visible
}
